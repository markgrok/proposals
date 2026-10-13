# Tasks

None is started. Group 0 is what 0.1.0 left open; groups 1 to 3 build `design.md` D1 to D4;
group 4 releases them. `reference/design.md` holds the detail each task needs.

## 0. Before: what 0.1.0 left open

- [ ] 0.1 MSVC 14.3 and 14.5 green. The superproject now loads every Jamfile on Windows (`path.exists` given native paths, `1e33cbc` and `c53f3f9`), and CI run 38365413330 then ran lexical on MSVC for the first time: `allocation` fails, its replaced global `operator new` counting 0 where the test expects 1 or 2 (`the_instrument_counts`, `every_alignment_is_counted_and_honoured`, `iris_and_templates_allocate_within_their_bounds`). To fix and see green
- [ ] 0.2 `AGENTS.md`'s bullet on the regex bounds still names `max_regex_alternatives`, which the validator no longer has: reword it
- [ ] 0.3 Measure the stack `is_regex` needs at `max_regex_nesting` on wasm32-wasip2 at `-O0` for the current validator, and state it in `AGENTS.md` and the page

## 1. The entry point and the strategies (design D1, D2)

- [ ] 1.1 Decide O1 (the `is_x` functions), O2 (composition) and O4 (names) with the owner
- [ ] 1.2 `<webcpp/lexical/is.hpp>`: `is`, `strategy_for`; compile-fail tests for a strategy of another format, a callable without a `string_view`, one without a `bool`
- [ ] 1.3 The format tags and their default strategies for every format, each with `rule()` where a rule exists; the default-equivalence and rule-equivalence tests over every string of the unit tests and the suite
- [ ] 1.4 The configured strategies: `time::rfc3339{leap}`, `date_time::rfc3339{leap}` (D10, with JSON Schema v1's `date-time` run with `refused`), `relative_json_pointer::draft{manipulation}`, `iri::rfc3987{resource}`, `iri_reference::rfc3987{resource}`, `regex::ecma262{max_length, max_nesting}`
- [ ] 1.5 `noexcept` follows the strategy: the `static_assert`s of `reference/design.md` section 10
- [ ] 1.6 The suite runner, the allocation test and the examples through `is<F>`; the `is_x` functions kept or removed as O1 decided
- [ ] 1.7 The page's chapter "Strategies", the reference, the changelog lines of the migration

## 2. Boost.Regex, optional (design D3)

- [ ] 2.1 Decide O3 (Boost.Regex as an optional dependency, recorded) and O5 (`ecma262_traits` in lexical) with the owner
- [ ] 2.2 The translator of the feasibility prototype rewritten to webcpp's rules: no `std::string` reason, output through a memory resource, lexical's own Unicode tables; its tests first, against the prototype's outputs
- [ ] 2.3 `ecma262_traits` over lexical's tables (classes, properties, simple case folding)
- [ ] 2.4 `<webcpp/lexical/regex/boost_regex.hpp>`: `boost_regex_pattern`, `regex::via_boost_regex<Judge>`, `boost_regex_compiles`; the include boundary's `except`; `/webcpp//boost_regex` for its tests and examples alone
- [ ] 2.5 Tests over the generated patterns `regex_v8` compares: the judge never called on a pattern V8 refuses; `boost_regex_compiles` agrees with V8 but on the committed list of `unsupported` patterns, checked to be exactly those
- [ ] 2.6 Allocation of `via_boost_regex` through its resource alone, measured and stated; every target, with and without exceptions

## 3. The matching interface json_schema needs (design D4)

- [ ] 3.1 Decide O6 (the interface in lexical or in json_schema) with the owner
- [ ] 3.2 Compile an ECMA-262 pattern into `basic_regex<char32_t, ecma262_traits>`, as a value that says why it failed (`syntax`, `unsupported`)
- [ ] 3.3 Search UTF-8 text with a step budget (10,000,000 steps plus 64 per byte by default), answering `found`, `not_found` or `limit`; hostile patterns and texts bounded on every target
- [ ] 3.4 Every regex-dependent test of the JSON-Schema-Test-Suite passing through it on every target, as the prototype did

## 4. The next version

- [ ] 4.1 The version's changelog, the page reviewed, the lint and the site checked locally, CI green on every lane, the tag
