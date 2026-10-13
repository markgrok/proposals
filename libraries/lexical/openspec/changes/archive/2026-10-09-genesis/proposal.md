## Why

A web program checks, at every boundary it does not control, that a string is a date, an e-mail
address, a host name, a URI or a regular expression before it uses it, and a JSON Schema
validator has to answer the same question for every `format` its schemas name. C++ had no
library that answers it for every string format JSON Schema names, exactly as the RFCs and
ECMA-262 define them, without allocating, without exceptions, natively and on WebAssembly.
webcpp::json_schema needs one for its `format` keyword.

## What Changes

- **New library `webcpp::lexical`** (developed as webcpp::formats and renamed on 2026-10-13,
  since "formats" read as Boost.Format or `std::format`), webcpp's own (not a port),
  header-only C++20: whether a string is a well-formed instance of each of the 19 string
  formats JSON Schema names (`date-time`, `date`, `time`, `duration`, `email`, `idn-email`,
  `hostname`, `idn-hostname`, `ipv4`, `ipv6`, `uri`, `uri-reference`, `iri`, `iri-reference`,
  `uri-template`, `json-pointer`, `relative-json-pointer`, `uuid`, `regex`).
- Each format is a validator `is_<name>(std::string_view)` and, but `regex`, a Boost.URL grammar
  rule in `webcpp::lexical::grammar` that a program composes with Boost.URL's combinators.
- Boost.URL 1.92 is guarded where it is more liberal than RFC 3986; IRIs are mapped to URIs;
  internationalized names go through webcpp::idna's public interface; `is_regex` is a syntax
  validator of lexical's own over Unicode 17.0.0 tables, allocating nothing.
- Proven by the official JSON-Schema-Test-Suite run against the functions, with known-failure
  lists that never lie, and by V8 for `regex`, on native, wasip2, wasip3 and Emscripten.

## Capabilities

### New Capabilities

- `lexical`: webcpp::lexical 0.1.0, the lexical forms of JSON Schema's string formats for C++20.

### Modified Capabilities

None.

## Impact

- **Kind:** a library of webcpp's own (`"port-of": null`).
- **Implementation:** https://github.com/webcpporg/lexical at `e286ca4`, the submodule
  `libs/lexical` of the superproject; `reference/source/` is that tree.
- **Dependencies:** Boost.URL (its compiled part, through `/webcpp//boost_url`), Boost.System,
  Boost's headers, webcpp::idna (public interface only) and webcpp::config. Test data: the
  superproject's JSON-Schema-Test-Suite submodule (MIT, never copied); Node for the V8 oracle.
- **Status:** implemented and published on 2026-10-13, in release 0.1.0 (`reference/STATUS.md`).
- **What follows:** the change `strategies` (one entry point, Boost.Regex as an optional
  strategy, the matching interface json_schema needs).
