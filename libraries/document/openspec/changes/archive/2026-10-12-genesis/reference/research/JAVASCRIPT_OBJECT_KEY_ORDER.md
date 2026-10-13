---
title: "JavaScript's object key order, and what xactor::document keeps"
tags: [webcpp, xactor, document, json, javascript]
status: active
created: 2026-10-12
---

# JavaScript's object key order

## The rule

ECMAScript's OrdinaryOwnPropertyKeys (ES2015 and later, section "OrdinaryOwnPropertyKeys") orders
an ordinary object's own keys in three groups:

1. **array indices**: keys that are the canonical decimal spelling of an integer from 0 to
   2^32 - 2, in ascending numeric order;
2. **every other string key**, in the order the property was created;
3. symbols, in creation order (JSON has none).

`Object.keys`, `JSON.stringify` (through EnumerableOwnProperties) and, since ES2020, `for-in`
follow it. `JSON.parse` creates the properties in source order, so the source order survives only
within group 2. A `Map` keeps pure insertion order for every key, numeric-looking or not.

## Measured (node 26.7.0, V8)

| What                                                   | Result                                                    |
| ------------------------------------------------------ | --------------------------------------------------------- |
| insert b, a, c                                         | `{"b":1,"a":2,"c":3}`                                     |
| assign an existing key                                 | keeps its place                                           |
| delete b, then add b                                   | b goes last                                               |
| create z, "10", "2", a, "1"                            | `{"1":5,"2":3,"10":2,"z":1,"a":4}`                        |
| `JSON.parse('{"b":1,"2":2,"a":3,"1":4}')`              | `{"1":4,"2":2,"b":1,"a":3}`                               |
| `"4294967294"` (an index) and `"4294967295"` (not one) | the first goes first, the second keeps its place          |
| `"01"`, `"-1"`, `"1.0"`, `" 1"`                        | not indices: creation order                               |
| a repeated key `{"a":1,"b":2,"a":3}`                   | `{"a":3,"b":2}`: the last value, in the first one's place |
| `Map` with b, "2", a, "1"                              | `['b','2','a','1']`                                       |

So JavaScript itself does not give back a JSON text's source order when integer-like keys follow
other keys.

## What this means for xactor::document

- A document that keeps source (insertion) order writes back exactly what it parsed.
- JSON produced by JavaScript (`JSON.stringify`) already has its integer-like keys first, so a
  document that keeps source order passes it on unchanged: nothing JavaScript sent changes order
  through webcpp.
- Only JSON written elsewhere, with integer-like keys after others, is ordered differently by a
  JavaScript peer that parses it, and the peer would do so anyway.
- JavaScript's own updates map onto the document's: assigning an existing member keeps its place;
  erasing and adding again puts it last.
- A repeated key: JavaScript keeps the last value; the document refuses it (`duplicate_key`).
  `JSON.stringify` never produces one.

## Boost.JSON's parser is not header-only by itself

`boost/json/basic_parser_impl.hpp` with a SAX handler and `number_precision::none` (no double
conversion) still needs five definitions from Boost.JSON's sources (spike
`~/.buzz/RESEARCH/WEBCPP_JSON_SAX_SPIKE.cpp`, Boost 1.92, native, wasip2, Emscripten):
`detail::stack::~stack`, `detail::stack::reserve_impl`, `detail::error_code_category`,
`detail::charconv::from_chars` (double) and `detail::throw_system_error`. Boost.JSON has no
header-only macro: its header-only use is `#include <boost/json/src.hpp>` in exactly one
translation unit, which webcpp already does as `tools/boost_json.cpp`, the static library
`/webcpp//boost_json` (pratt and trystero link it). A header-only library cannot include `src.hpp`
in its headers (two translation units would define the same symbols).
