## Why

lexical 0.1.0's 22 functions have four shapes: `is_x(s)` for 17 of them,
`is_time(s, leap_seconds)`, `is_relative_json_pointer(s, index_manipulation)`, and `is_iri(s)` /
`is_iri(s, memory_resource&)` with the same pair for `is_iri_reference`. A program cannot treat
the formats alike (a table of formats needs a wrapper per format), cannot choose another
implementation of a format, and must accept lexical's own regex validator. `is_date_time`
cannot refuse second 60, which JSON Schema v1 asks. And webcpp::json_schema's regex policy
needs to compile and search ECMA-262 patterns, which lexical, a validator, does not offer.

## What Changes

- **One entry point:** `lexical::is<Format>(std::string_view s, Strategy const& strategy =
  Format::default_strategy{})` for every format, any callable `bool(std::string_view)` accepted
  as a strategy, the strategies webcpp ships bound to their format.
- **Configuration moves into strategies:** `time::rfc3339{.leap}`, `date_time::rfc3339{.leap}`
  (new), `relative_json_pointer::draft{.manipulation}`, `iri::rfc3987{.resource}`,
  `regex::ecma262{.max_length, .max_nesting}`; each shipped strategy gives its grammar rule.
- **Boost.Regex, optional:** `regex::via_boost_regex<Judge>` translates an ECMA-262 pattern
  into Boost.Regex's syntax and hands it to a judge; `boost_regex_compiles` and
  `ecma262_traits`; one header alone includes Boost.Regex.
- **The matching interface json_schema needs:** compile an ECMA-262 pattern for Boost.Regex,
  search UTF-8 text with a step budget.
- **BREAKING (open question O1):** the 22 `is_x` functions may leave.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `lexical`: adds the entry point, the strategies, the Boost.Regex header and the matching
  interface.

## Impact

- **Kind:** a change to a library of webcpp's own.
- **Implementation:** not started. `reference/design.md` is the design, written on 2026-10-13
  from the owner's request; `reference/prototype/` is the ECMA-262 to Boost.Regex translator of
  the feasibility study (`reference/feasibility.md`: identical to V8 on 20,130 fuzzed patterns,
  every regex test of the JSON-Schema-Test-Suite passing on five targets), which tasks 2.2 and
  3.x rewrite to webcpp's rules.
- **Status:** draft: designed, decisions O1 to O6 open, nothing built.
- **Dependencies:** Boost.Regex 1.92, header-only, for one optional header (O3).
- **Other proposals:** json_schema's genesis (its task 1.4) waits for group 3.
