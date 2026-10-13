---
title: "Boost.Regex, Boost.URL and Boost.Locale against JSON Schema"
tags: [webcpp, json-schema, spike]
status: superseded
created: 2026-10-08
---

> **Superseded on 2026-10-12.** The owner abandoned SRELL, webcpp::regex, webcpp::json_schema, webcpp::formats, webcpp::idna and webcpp::zod (PLANS/WEBCPP_DESIGN.md, 2026-10-12). Kept for the record only.

# Boost.Regex, Boost.URL and Boost.Locale against JSON Schema

A measurement spike for `webcpp::json_schema`. The owner's rule: Boost.Regex
and Boost.URL are the default engines, behind template policies, unless more
than 10% of the official JSON-Schema-Test-Suite tests that depend on the engine
fail; then an existing library is vendored instead. Background:
`~/.buzz/RESEARCH/JSON_SCHEMA_FOR_WEBCPP.md`, sections 4 (formats) and 5
(regex), whose findings this spike measures end to end.

Everything ran on 2026-10-08, macOS on Apple silicon, and is throwaway under
`~/.claude/jobs/fcf3a902/tmp/spike-regex/` (called `$S` below);
`$S/run.sh` rebuilds and re-measures all of it. Inputs:

| Input | Version |
| --- | --- |
| JSON-Schema-Test-Suite | commit `5b0ee16`, clone at `~/.claude/jobs/fcf3a902/tmp/jsonschema-research/JSON-Schema-Test-Suite` |
| Boost | 1.92 from Homebrew (`/opt/homebrew/opt/boost`); Boost.URL, Boost.Locale and Boost.Charconv sources from the boostorg tags `boost-1.92.0` (url `a34f3a4`, locale `94f4d99`) |
| ICU | icu4c 78 from Homebrew (`/opt/homebrew/opt/icu4c@78`), for `boost::u32regex` |
| SRELL | 2026.07 (2026-09-17, Unicode 18.0), https://www.akenotsuki.com/misc/srell/releases/srell2026_07.zip, SHA-256 `d384feae20c66b4d65bc240ee2a851aed26ca24dc4d73503eee3e81e238ec20e` |
| QuickJS-ng libregexp | https://github.com/quickjs-ng/quickjs commit `dad13e3` (2026-10-08) |
| PCRE2 | 10.47 from Homebrew |
| Native compiler | Apple clang, libc++, `-std=c++20 -O2` |
| WASI | wasi-sdk 34 at `~/dev/webcpp/.local/wasi-sdk`, wasmtime 47.0.3; flags from `~/dev/webcpp/Jamroot`: wasip2 `-fno-exceptions -DBOOST_NO_EXCEPTIONS`, wasip3 `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false` and `-lunwind`, run with `wasmtime run -W exceptions=y` |
| Reference | node 26.7 (V8), `new RegExp(pattern, "u")`, ECMA-262 itself |

## Summary

| Library | Measure | Result | Against the 10% rule |
| --- | --- | --- | --- |
| Boost.Regex (narrow `boost::regex`, any option set) | required regex-dependent tests | 2020-12: 7.1% (5/70); v1: 15.7% (13/83); all six drafts: 5.6% (18/323) | under 10% for 2020-12 and overall, **over for v1** |
| Boost.Regex (narrow) | required + optional | 2020-12: 19.4% (33/170); all drafts: 18.7% (165/881) | **over** |
| Boost.Regex `u32regex` + ICU | required; required + optional | 2020-12: 0% and 12.4%; all drafts: 2.2% and 12.7% | required under, **required + optional over** |
| Boost.Regex `u32regex` + ICU + an ECMA-262 translation layer | required; required + optional | 2020-12: 0% and 0.6%; all drafts: 0.3% and 0.5% | under, but ICU is a compiled dependency and matching still aborts without exceptions |
| Boost.URL | `uri` + `uri-reference` | 2.7% (10/371), all drafts | **under** |
| Boost.URL | adding `iri`, `iri-reference` | 11.4% (58/511) | over: IRIs are not URIs, Boost.URL rejects every non-ASCII character |
| Boost.URL | `uri-template` | 100% (205/205): no RFC 6570 parser | out of its scope |
| Boost.Locale | wasip2 without exceptions | does not compile: 17 of 28 sources, and `<boost/locale.hpp>` itself | unusable under the wasip2 policy |
| Boost.Locale | wasip3 (and wasip2 with exceptions), posix or std backend, wasi-libc iconv | compiles, links, runs: `translate` with a `.mo` catalog, plurals, `format`, `to_upper`, charset conversion | works, but only the C locale's number and date formats |
| SRELL 2026.07 with `SRELL_NO_UBMOD` | required; required + optional | 0% and 0% (881/881, the same as V8) | under |
| QuickJS-ng libregexp | required; required + optional | 0% and 0% (881/881) | under |

Boost.Regex is over 10% on required plus optional in every draft, and on
required alone for v1, whose `format/` tests are required. Its failures are
structural, not configuration: narrow `boost::regex` works on UTF-8 bytes and
has no Unicode properties, ECMAScript is a synonym of perl, and a match that
reaches the complexity limit calls `boost::throw_exception`, an abort without
exceptions, with no error-value API. The rule therefore sends regex to a
vendored engine. **SRELL** is the recommended default: header-only, BSD-2,
ECMAScript by design, passes every regex-dependent test, reports a bad
pattern and a complexity overrun as error values (`ecode()`), and builds and
runs on wasip2 without exceptions after a four-line patch. QuickJS-ng's
libregexp is the second choice (MIT, C, also 881/881, a step-budget hook), not
header-only and UTF-16 input only. Boost.URL stays the default for `uri` and
`uri-reference`; `iri`, `iri-reference` and `uri-template` need parsers of
their own. Boost.Locale is not a dependency to take for the wasip2 lane.

## 1. Method

`$S/suite.py` walks every test file of the six drafts (`draft4`, `draft6`,
`draft7`, `draft2019-09`, `draft2020-12`, `v1`) with a minimal evaluator of
the keywords that occur next to `pattern` and `patternProperties` in the
suite: `type`, `enum`, `const`, `pattern`, `format`, `minimum`, `maximum`,
`minItems`, `maxItems`, `properties`, `patternProperties`,
`additionalProperties`, `propertyNames`, `required`, `allOf`, `anyOf`,
`oneOf`, `not`, `if`/`then`/`else`, local `$ref`, and
`unevaluatedProperties` with annotation collection. Every place where a regex
decides something becomes a query to the engine:

- `pattern`: does the pattern match somewhere in the string (a search, not a
  full match);
- `patternProperties` (and `propertyNames` with `pattern`): the same, per
  property name;
- `format: "regex"`: does the string compile as a pattern. Asserted only
  where the suite asserts formats: `optional/format/` in every draft, and
  `format/` in v1, where format assertion is the default and those tests are
  required. Elsewhere (`format.json`, v1's `optional/format-annotation.json`)
  `format` is an annotation and no query is made.

The evaluator never short-circuits, so the queries of a test do not depend on
the answers. **A test is regex-dependent when it makes at least one query.**
The C++ harness (`$S/harness.cpp`) answers the queries with one engine
configuration; Python then evaluates the test with those answers. Regex is
therefore the only thing judged: the structural keywords are evaluated the
same way for every engine, which isolates the regex part of tests such as
`properties.json`'s "properties, patternProperties, additionalProperties
interaction" and the 21 `unevaluatedProperties` tests with `$ref` and
`oneOf`, instead of skipping them. Skipped: the 4 tests of
`draft2019-09/recursiveRef.json` whose schema has `$recursiveAnchor`, the
only keyword met that the evaluator does not implement.

Verdicts: **passed**, **failed** (wrong verdict), **error** (a `pattern` or
`patternProperties` regex did not compile, or matching threw or returned an
error; for `format: "regex"` a pattern that does not compile is the verdict
"invalid", not an error), **skipped**. Fail % = (failed + errors) / (total -
skipped). Required = files outside `optional/`, plus v1's `format/`.

The evaluator was validated against V8 (`$S/harness.mjs`, `new RegExp(p,
"u")`): **881 of 881 judged tests pass**, so every failure below belongs to
the engine, not to the evaluator. 885 regex-dependent tests in all, 142
distinct queries:

| Draft | Required files (tests) | Optional files (tests) |
| --- | --- | --- |
| draft4 | additionalProperties 5, pattern 3, patternProperties 15, properties 8 | ecmascript-regex 74, non-bmp-regex 12 |
| draft6 | the same + propertyNames 2, patternProperties 19 | ecmascript-regex 74, non-bmp-regex 12 |
| draft7 | as draft6 | ecmascript-regex 74, non-bmp-regex 12, format/ecmascript-regex 12, format/regex 2 |
| draft2019-09 | as draft7 + recursiveRef 4 (skipped), unevaluatedProperties 28 | as draft7 |
| draft2020-12 | additionalProperties 5, pattern 6, patternProperties 21, properties 8, propertyNames 2, unevaluatedProperties 28 | as draft7 |
| v1 | additionalProperties 5, pattern 6, patternProperties 22, properties 8, unevaluatedProperties 28, format/ecmascript-regex 12, format/regex 2 | ecmascript-regex 74, non-bmp-regex 12 |

Of the 70 required regex-dependent tests of 2020-12, 65 use ASCII patterns
on ASCII-or-literal data that any engine passes; the 5 that are not are the
`^\p{Letter}+$` tests. The required measure is diluted accordingly; the
optional files are where ECMA-262 semantics are tested.

## 2. Boost.Regex against the regex-dependent tests

### 2.1 Configurations

| Name | Engine and options |
| --- | --- |
| `boost-perl` | `boost::regex(p, perl \| no_except)`, `regex_search(s, re)`, UTF-8 bytes |
| `boost-ecma` | `ECMAScript \| no_except`. `boost/regex/v5/regbase.hpp` defines `perl = normal, ECMAScript = normal, JavaScript = normal`: the same flags, so the same results |
| `boost-ecma-nomod` | `ECMAScript \| no_mod_m \| no_mod_s \| no_except`, and `match_not_dot_newline` |
| `boost-ecma-nomod-tr` | the same after a prototype ECMA-262 to Boost translation (`translate()` in `$S/harness.cpp`, UTF-8-byte aware): `\d \w \s` and their negations made explicit ASCII or ECMA WhiteSpace classes, `^` to `\A`, `$` to `\z`, `.` excluding line terminators, `\uXXXX`, surrogate pairs and `\u{X}` to code points, `\cX`, `[]` to `(?!)`, `[^]` to any code point, multi-byte literals grouped, negated classes made code-point aware; and syntax ECMA-262 rejects refused (`\a` and other unknown escapes, `(?P`, `(?#`, `(?i)`, possessive quantifiers). `\p{...}` cannot be expressed on bytes and is refused |
| `u32-perl` | `boost::make_u32regex(p, perl \| no_except)`, `u32regex_search` on UTF-8 (ICU 78, `boost/regex/icu.hpp`) |
| `u32-nomod` | `u32regex` with `no_mod_m \| no_mod_s` and `match_not_dot_newline` |
| `u32-nomod-tr` | `u32regex` after the same translation, with code points instead of bytes; `\p{...}` passed through to ICU |

For comparison, on the same harness: `srell` (SRELL `u8cregex`, ECMAScript),
`srell-noubmod` (built with `SRELL_NO_UBMOD`), `qjs` (QuickJS-ng libregexp,
`LRE_FLAG_UNICODE`, UTF-16 subject), `pcre2` (`PCRE2_UTF | PCRE2_UCP |
PCRE2_DOLLAR_ENDONLY | PCRE2_ALLOW_EMPTY_CLASS | PCRE2_ALT_BSUX`, extra
`PCRE2_EXTRA_ALT_BSUX | PCRE2_EXTRA_ASCII_BSD | PCRE2_EXTRA_ASCII_BSW`, no
translation), `std-ecma` (libc++ `std::regex::ECMAScript`), `v8-u` (the
reference).

### 2.2 Fail % per configuration

| engine | draft4 req | draft4 all | draft6 req | draft6 all | draft7 req | draft7 all | draft2019-09 req | draft2019-09 all | draft2020-12 req | draft2020-12 all | v1 req | v1 all | overall req | overall all |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| `boost-perl` | 0.0% | 17.9% | 0.0% | 17.1% | 0.0% | 20.4% | 0.0% | 17.0% | 7.1% | 19.4% | 15.7% | 20.1% | 5.6% | 18.7% |
| `boost-ecma` | 0.0% | 17.9% | 0.0% | 17.1% | 0.0% | 20.4% | 0.0% | 17.0% | 7.1% | 19.4% | 15.7% | 20.1% | 5.6% | 18.7% |
| `boost-ecma-nomod` | 0.0% | 17.9% | 0.0% | 17.1% | 0.0% | 20.4% | 0.0% | 17.0% | 7.1% | 19.4% | 15.7% | 20.1% | 5.6% | 18.7% |
| `boost-ecma-nomod-tr` | 0.0% | 12.0% | 0.0% | 11.4% | 0.0% | 10.9% | 0.0% | 9.1% | 7.1% | 11.8% | 8.4% | 12.4% | 3.7% | 11.2% |
| `u32-perl` | 0.0% | 12.0% | 0.0% | 11.4% | 0.0% | 15.3% | 0.0% | 12.7% | 0.0% | 12.4% | 8.4% | 12.4% | 2.2% | 12.7% |
| `u32-nomod` | 0.0% | 12.0% | 0.0% | 11.4% | 0.0% | 15.3% | 0.0% | 12.7% | 0.0% | 12.4% | 8.4% | 12.4% | 2.2% | 12.7% |
| `u32-nomod-tr` | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.7% | 0.0% | 0.6% | 0.0% | 0.6% | 1.2% | 0.6% | 0.3% | 0.5% |
| `srell` | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 1.5% | 0.0% | 1.2% | 0.0% | 1.2% | 2.4% | 1.2% | 0.6% | 0.9% |
| `srell-noubmod` | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% |
| `qjs` | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% |
| `pcre2` | 0.0% | 13.7% | 0.0% | 13.0% | 0.0% | 16.8% | 0.0% | 13.9% | 7.1% | 16.5% | 15.7% | 17.2% | 5.6% | 15.3% |
| `std-ecma` | 0.0% | 21.4% | 0.0% | 20.3% | 0.0% | 20.4% | 0.0% | 17.0% | 7.1% | 19.4% | 10.8% | 20.1% | 4.3% | 19.6% |
| `v8-u` | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% | 0.0% |

`no_mod_m`/`no_mod_s` change nothing on the suite: its only `$` test,
"ECMA 262 regex $ does not match trailing newline", has the data `"abc\\n"`
in JSON, a literal backslash and `n`, not a newline (`tests/draft2020-12/optional/ecmascript-regex.json`
line 12), and no test puts a newline in front of `.`, `^` or `$`. That is a
gap in the suite worth reporting upstream. Section 2.6 shows the flags do
matter outside it.

### 2.3 Per draft

Columns: total, passed, failed, errors, skipped and fail %, for the required
tests and for required plus optional.

#### `boost-ecma-nomod`

| draft | req total | req pass | req fail | req err | req skip | req fail % | all total | all pass | all fail | all err | all skip | all fail % |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| draft4 | 31 | 31 | 0 | 0 | 0 | 0.0% | 117 | 96 | 13 | 8 | 0 | 17.9% |
| draft6 | 37 | 37 | 0 | 0 | 0 | 0.0% | 123 | 102 | 13 | 8 | 0 | 17.1% |
| draft7 | 37 | 37 | 0 | 0 | 0 | 0.0% | 137 | 109 | 20 | 8 | 0 | 20.4% |
| draft2019-09 | 69 | 65 | 0 | 0 | 4 | 0.0% | 169 | 137 | 20 | 8 | 4 | 17.0% |
| draft2020-12 | 70 | 65 | 0 | 5 | 0 | 7.1% | 170 | 137 | 20 | 13 | 0 | 19.4% |
| v1 | 83 | 70 | 7 | 6 | 0 | 15.7% | 169 | 135 | 20 | 14 | 0 | 20.1% |
| overall | 327 | 305 | 7 | 11 | 4 | 5.6% | 885 | 716 | 106 | 59 | 4 | 18.7% |

#### `boost-ecma-nomod-tr`

| draft | req total | req pass | req fail | req err | req skip | req fail % | all total | all pass | all fail | all err | all skip | all fail % |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| draft4 | 31 | 31 | 0 | 0 | 0 | 0.0% | 117 | 103 | 0 | 14 | 0 | 12.0% |
| draft6 | 37 | 37 | 0 | 0 | 0 | 0.0% | 123 | 109 | 0 | 14 | 0 | 11.4% |
| draft7 | 37 | 37 | 0 | 0 | 0 | 0.0% | 137 | 122 | 1 | 14 | 0 | 10.9% |
| draft2019-09 | 69 | 65 | 0 | 0 | 4 | 0.0% | 169 | 150 | 1 | 14 | 4 | 9.1% |
| draft2020-12 | 70 | 65 | 0 | 5 | 0 | 7.1% | 170 | 150 | 1 | 19 | 0 | 11.8% |
| v1 | 83 | 76 | 1 | 6 | 0 | 8.4% | 169 | 148 | 1 | 20 | 0 | 12.4% |
| overall | 327 | 311 | 1 | 11 | 4 | 3.7% | 885 | 782 | 4 | 95 | 4 | 11.2% |

#### `u32-nomod`

| draft | req total | req pass | req fail | req err | req skip | req fail % | all total | all pass | all fail | all err | all skip | all fail % |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| draft4 | 31 | 31 | 0 | 0 | 0 | 0.0% | 117 | 103 | 14 | 0 | 0 | 12.0% |
| draft6 | 37 | 37 | 0 | 0 | 0 | 0.0% | 123 | 109 | 14 | 0 | 0 | 11.4% |
| draft7 | 37 | 37 | 0 | 0 | 0 | 0.0% | 137 | 116 | 21 | 0 | 0 | 15.3% |
| draft2019-09 | 69 | 65 | 0 | 0 | 4 | 0.0% | 169 | 144 | 21 | 0 | 4 | 12.7% |
| draft2020-12 | 70 | 70 | 0 | 0 | 0 | 0.0% | 170 | 149 | 21 | 0 | 0 | 12.4% |
| v1 | 83 | 76 | 7 | 0 | 0 | 8.4% | 169 | 148 | 21 | 0 | 0 | 12.4% |
| overall | 327 | 316 | 7 | 0 | 4 | 2.2% | 885 | 769 | 112 | 0 | 4 | 12.7% |

#### `u32-nomod-tr`

| draft | req total | req pass | req fail | req err | req skip | req fail % | all total | all pass | all fail | all err | all skip | all fail % |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| draft4 | 31 | 31 | 0 | 0 | 0 | 0.0% | 117 | 117 | 0 | 0 | 0 | 0.0% |
| draft6 | 37 | 37 | 0 | 0 | 0 | 0.0% | 123 | 123 | 0 | 0 | 0 | 0.0% |
| draft7 | 37 | 37 | 0 | 0 | 0 | 0.0% | 137 | 136 | 1 | 0 | 0 | 0.7% |
| draft2019-09 | 69 | 65 | 0 | 0 | 4 | 0.0% | 169 | 164 | 1 | 0 | 4 | 0.6% |
| draft2020-12 | 70 | 70 | 0 | 0 | 0 | 0.0% | 170 | 169 | 1 | 0 | 0 | 0.6% |
| v1 | 83 | 82 | 1 | 0 | 0 | 1.2% | 169 | 168 | 1 | 0 | 0 | 0.6% |
| overall | 327 | 322 | 1 | 0 | 4 | 0.3% | 885 | 877 | 4 | 0 | 4 | 0.5% |

#### `srell`

| draft | req total | req pass | req fail | req err | req skip | req fail % | all total | all pass | all fail | all err | all skip | all fail % |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| draft4 | 31 | 31 | 0 | 0 | 0 | 0.0% | 117 | 117 | 0 | 0 | 0 | 0.0% |
| draft6 | 37 | 37 | 0 | 0 | 0 | 0.0% | 123 | 123 | 0 | 0 | 0 | 0.0% |
| draft7 | 37 | 37 | 0 | 0 | 0 | 0.0% | 137 | 135 | 2 | 0 | 0 | 1.5% |
| draft2019-09 | 69 | 65 | 0 | 0 | 4 | 0.0% | 169 | 163 | 2 | 0 | 4 | 1.2% |
| draft2020-12 | 70 | 70 | 0 | 0 | 0 | 0.0% | 170 | 168 | 2 | 0 | 0 | 1.2% |
| v1 | 83 | 81 | 2 | 0 | 0 | 2.4% | 169 | 167 | 2 | 0 | 0 | 1.2% |
| overall | 327 | 321 | 2 | 0 | 4 | 0.6% | 885 | 873 | 8 | 0 | 4 | 0.9% |

#### `srell-noubmod`

| draft | req total | req pass | req fail | req err | req skip | req fail % | all total | all pass | all fail | all err | all skip | all fail % |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| draft4 | 31 | 31 | 0 | 0 | 0 | 0.0% | 117 | 117 | 0 | 0 | 0 | 0.0% |
| draft6 | 37 | 37 | 0 | 0 | 0 | 0.0% | 123 | 123 | 0 | 0 | 0 | 0.0% |
| draft7 | 37 | 37 | 0 | 0 | 0 | 0.0% | 137 | 137 | 0 | 0 | 0 | 0.0% |
| draft2019-09 | 69 | 65 | 0 | 0 | 4 | 0.0% | 169 | 165 | 0 | 0 | 4 | 0.0% |
| draft2020-12 | 70 | 70 | 0 | 0 | 0 | 0.0% | 170 | 170 | 0 | 0 | 0 | 0.0% |
| v1 | 83 | 83 | 0 | 0 | 0 | 0.0% | 169 | 169 | 0 | 0 | 0 | 0.0% |
| overall | 327 | 323 | 0 | 0 | 4 | 0.0% | 885 | 881 | 0 | 0 | 4 | 0.0% |

#### `qjs`

| draft | req total | req pass | req fail | req err | req skip | req fail % | all total | all pass | all fail | all err | all skip | all fail % |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| draft4 | 31 | 31 | 0 | 0 | 0 | 0.0% | 117 | 117 | 0 | 0 | 0 | 0.0% |
| draft6 | 37 | 37 | 0 | 0 | 0 | 0.0% | 123 | 123 | 0 | 0 | 0 | 0.0% |
| draft7 | 37 | 37 | 0 | 0 | 0 | 0.0% | 137 | 137 | 0 | 0 | 0 | 0.0% |
| draft2019-09 | 69 | 65 | 0 | 0 | 4 | 0.0% | 169 | 165 | 0 | 0 | 4 | 0.0% |
| draft2020-12 | 70 | 70 | 0 | 0 | 0 | 0.0% | 170 | 170 | 0 | 0 | 0 | 0.0% |
| v1 | 83 | 83 | 0 | 0 | 0 | 0.0% | 169 | 169 | 0 | 0 | 0 | 0.0% |
| overall | 327 | 323 | 0 | 0 | 4 | 0.0% | 885 | 881 | 0 | 0 | 4 | 0.0% |

#### `pcre2`

| draft | req total | req pass | req fail | req err | req skip | req fail % | all total | all pass | all fail | all err | all skip | all fail % |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| draft4 | 31 | 31 | 0 | 0 | 0 | 0.0% | 117 | 101 | 2 | 14 | 0 | 13.7% |
| draft6 | 37 | 37 | 0 | 0 | 0 | 0.0% | 123 | 107 | 2 | 14 | 0 | 13.0% |
| draft7 | 37 | 37 | 0 | 0 | 0 | 0.0% | 137 | 114 | 9 | 14 | 0 | 16.8% |
| draft2019-09 | 69 | 65 | 0 | 0 | 4 | 0.0% | 169 | 142 | 9 | 14 | 4 | 13.9% |
| draft2020-12 | 70 | 65 | 0 | 5 | 0 | 7.1% | 170 | 142 | 9 | 19 | 0 | 16.5% |
| v1 | 83 | 70 | 7 | 6 | 0 | 15.7% | 169 | 140 | 9 | 20 | 0 | 17.2% |
| overall | 327 | 305 | 7 | 11 | 4 | 5.6% | 885 | 746 | 40 | 95 | 4 | 15.3% |

#### `std-ecma`

| draft | req total | req pass | req fail | req err | req skip | req fail % | all total | all pass | all fail | all err | all skip | all fail % |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| draft4 | 31 | 31 | 0 | 0 | 0 | 0.0% | 117 | 92 | 11 | 14 | 0 | 21.4% |
| draft6 | 37 | 37 | 0 | 0 | 0 | 0.0% | 123 | 98 | 11 | 14 | 0 | 20.3% |
| draft7 | 37 | 37 | 0 | 0 | 0 | 0.0% | 137 | 109 | 14 | 14 | 0 | 20.4% |
| draft2019-09 | 69 | 65 | 0 | 0 | 4 | 0.0% | 169 | 137 | 14 | 14 | 4 | 17.0% |
| draft2020-12 | 70 | 65 | 0 | 5 | 0 | 7.1% | 170 | 137 | 14 | 19 | 0 | 19.4% |
| v1 | 83 | 74 | 3 | 6 | 0 | 10.8% | 169 | 135 | 14 | 20 | 0 | 20.1% |
| overall | 327 | 309 | 3 | 11 | 4 | 4.3% | 885 | 708 | 78 | 95 | 4 | 19.6% |

### 2.4 Per file, 2020-12 and v1

Cells: passed/failed/errors/skipped of total.

#### draft2020-12

| file | kind | `boost-ecma-nomod` | `boost-ecma-nomod-tr` | `u32-nomod` | `u32-nomod-tr` | `srell` | `srell-noubmod` | `qjs` | `pcre2` | `std-ecma` |
|---|---|---|---|---|---|---|---|---|---|---|
| additionalProperties.json | required | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 |
| pattern.json | required | 3/0/3/0 of 6 | 3/0/3/0 of 6 | 6/0/0/0 of 6 | 6/0/0/0 of 6 | 6/0/0/0 of 6 | 6/0/0/0 of 6 | 6/0/0/0 of 6 | 3/0/3/0 of 6 | 3/0/3/0 of 6 |
| patternProperties.json | required | 19/0/2/0 of 21 | 19/0/2/0 of 21 | 21/0/0/0 of 21 | 21/0/0/0 of 21 | 21/0/0/0 of 21 | 21/0/0/0 of 21 | 21/0/0/0 of 21 | 19/0/2/0 of 21 | 19/0/2/0 of 21 |
| properties.json | required | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 |
| propertyNames.json | required | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 |
| unevaluatedProperties.json | required | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 |
| optional/ecmascript-regex.json | optional | 56/10/8/0 of 74 | 60/0/14/0 of 74 | 60/14/0/0 of 74 | 74/0/0/0 of 74 | 74/0/0/0 of 74 | 74/0/0/0 of 74 | 74/0/0/0 of 74 | 58/2/14/0 of 74 | 52/8/14/0 of 74 |
| optional/format/ecmascript-regex.json | optional | 5/7/0/0 of 12 | 11/1/0/0 of 12 | 5/7/0/0 of 12 | 11/1/0/0 of 12 | 10/2/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 5/7/0/0 of 12 | 9/3/0/0 of 12 |
| optional/format/regex.json | optional | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 |
| optional/non-bmp-regex.json | optional | 9/3/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 9/3/0/0 of 12 |

#### v1

| file | kind | `boost-ecma-nomod` | `boost-ecma-nomod-tr` | `u32-nomod` | `u32-nomod-tr` | `srell` | `srell-noubmod` | `qjs` | `pcre2` | `std-ecma` |
|---|---|---|---|---|---|---|---|---|---|---|
| additionalProperties.json | required | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 | 5/0/0/0 of 5 |
| format/ecmascript-regex.json | required | 5/7/0/0 of 12 | 11/1/0/0 of 12 | 5/7/0/0 of 12 | 11/1/0/0 of 12 | 10/2/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 5/7/0/0 of 12 | 9/3/0/0 of 12 |
| format/regex.json | required | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 | 2/0/0/0 of 2 |
| pattern.json | required | 3/0/3/0 of 6 | 3/0/3/0 of 6 | 6/0/0/0 of 6 | 6/0/0/0 of 6 | 6/0/0/0 of 6 | 6/0/0/0 of 6 | 6/0/0/0 of 6 | 3/0/3/0 of 6 | 3/0/3/0 of 6 |
| patternProperties.json | required | 19/0/3/0 of 22 | 19/0/3/0 of 22 | 22/0/0/0 of 22 | 22/0/0/0 of 22 | 22/0/0/0 of 22 | 22/0/0/0 of 22 | 22/0/0/0 of 22 | 19/0/3/0 of 22 | 19/0/3/0 of 22 |
| properties.json | required | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 | 8/0/0/0 of 8 |
| unevaluatedProperties.json | required | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 | 28/0/0/0 of 28 |
| optional/ecmascript-regex.json | optional | 56/10/8/0 of 74 | 60/0/14/0 of 74 | 60/14/0/0 of 74 | 74/0/0/0 of 74 | 74/0/0/0 of 74 | 74/0/0/0 of 74 | 74/0/0/0 of 74 | 58/2/14/0 of 74 | 52/8/14/0 of 74 |
| optional/non-bmp-regex.json | optional | 9/3/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 12/0/0/0 of 12 | 9/3/0/0 of 12 |

### 2.5 Failing patterns and why

Narrow `boost::regex` (`boost-perl`, `boost-ecma`, `boost-ecma-nomod`: the
same 165 failing results over six drafts, 44 distinct tests):

| Pattern | Tests (required) | Data that fails | Reason |
| --- | --- | --- | --- |
| `^\p{Letter}+$` (pattern.json, patternProperties.json) | 11 (11) | every case, ASCII included | does not compile, `error_ctype`: Boost's `\p{name}` is a POSIX class name (`\p{digit}` is `[[:digit:]]`), there are no Unicode properties on narrow strings |
| `\p{Letter}cole` | 48 (0) | every case | the same |
| `^\p{digit}+$` | 12 (0) | Bengali digits U+09EA U+09E8 | compiles as `[[:digit:]]`, ASCII only; the suite (and V8, SRELL, QuickJS) want Unicode Nd |
| `^\D$` | 12 (0) | NKO DIGIT ZERO U+07C0 | bytes: `\D` matches one of its two UTF-8 bytes, `$` then fails |
| `^\W$` | 6 (0) | `é` | bytes, as above |
| `^\s$` | 24 (0) | U+00A0, U+FEFF, U+2029, U+2003 | bytes, and Boost's `\s` is not ECMA WhiteSpace + LineTerminator |
| `^\S$` | 6 (0) | U+2013 | bytes |
| `^🐲*$` | 18 (0) | `""`, two dragons, a two-dragon property name | the quantifier applies to the last of the emoji's 4 bytes |
| `format: regex` `\a` | 4 (1) | accepted | Boost knows `\a` (bell); ECMA-262 u-mode rejects it |
| `format: regex` `(?#comment)a` | 4 (1) | accepted | perl comment group |
| `format: regex` `(?i)abc`, `(?ims)abc` | 8 (2) | accepted | perl inline flags |
| `format: regex` `(?<=a+)b` | 4 (1) | rejected, `error_bad_pattern` | variable-width lookbehind is not supported |
| `format: regex` `[]`, `[^]` | 8 (2) | rejected, `error_brack` | an empty class is not allowed |

`boost-ecma-nomod-tr`, narrow with the translation layer (99 failing results):
every non-`\p` failure above is fixed except `(?<=a+)b`; the 95 errors left
are `\p{Letter}cole`, `^\p{Letter}+$` (required) and `^\p{digit}+$`, which no
byte-level translation can express.

`u32regex` + ICU (`u32-perl`, `u32-nomod`: 112 failing results) fixes `\p`,
non-BMP and `\s` on NBSP, and fails the other way:

| Pattern | Tests | Data | Reason |
| --- | --- | --- | --- |
| `^\d$`, `^\D$`, `^\d+$` | 36 | U+07C0, Bengali digits | ICU's `\d` is Unicode Nd; ECMA-262's is `[0-9]` |
| `^\w$`, `^\W$`, `\wcole` | 36 | `é`, `l'école` | ICU's `\w` is Unicode; ECMA-262's is `[A-Za-z0-9_]` |
| `^\s$`, `^\S$` | 12 | U+FEFF | ICU does not count U+FEFF as space; ECMA-262 does |
| `format: regex` `\a`, `(?#comment)a`, `(?i)abc`, `(?ims)abc` | 16 | accepted | perl syntax |
| `format: regex` `(?<=a+)b`, `[]`, `[^]` | 12 | rejected | as narrow |

`u32-nomod-tr` (4 failing results): only `format: regex` `(?<=a+)b`, one
required test in v1 and one optional test in each of draft7, 2019-09 and
2020-12. Boost.Regex has no variable-width lookbehind at all.

Appendix A lists every failing test of each configuration.

### 2.6 ECMA-262 behaviour the suite does not test

`$S/extra.py`, answers 1 (match or compiles), 0, E (error):

| Query | V8 | boost-ecma | boost nomod | boost nomod+tr | u32 nomod | u32 nomod+tr | SRELL (NO_UBMOD) | QuickJS | PCRE2 | std::regex |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `^abc$` on `abc` + LF | 0 | **1** | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| `^abc$` on `x` + LF + `abc` | 0 | **1** | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| `^a.c$` on `a` + LF + `c` | 0 | **1** | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| `^a.c$` on `a` U+2028 `c` | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | **1** | 0 |
| `^.$` on U+1F432 | 1 | **0** | **0** | 1 | 1 | 1 | 1 | 1 | 1 | **0** |
| `^[^a]$` on `é` | 1 | **0** | **0** | 1 | 1 | 1 | 1 | 1 | 1 | **0** |
| `A` on `A` | 1 | **0** | **0** | 1 | **0** | 1 | 1 | 1 | 1 | 1 |
| `\u{1F432}` on U+1F432 | 1 | **0** | **0** | 1 | **0** | 1 | 1 | 1 | 1 | **E** |
| `^\p{Lu}+$` on `ÉCOLE` | 1 | **E** | **E** | **E** | 1 | 1 | 1 | 1 | 1 | **E** |
| `^\p{Script=Greek}+$` on `πλ` | 1 | **E** | **E** | **E** | **E** | **E** | 1 | 1 | 1 | **E** |
| `(?<=\$\d*)x` on `$12x` | 1 | **E** | **E** | **E** | **E** | **E** | 1 | 1 | **E** | **E** |
| compiles `a{,5}` | 0 | **1** | **1** | **1** | **1** | **1** | 0 | 0 | **1** | E |
| compiles `\Z` | 0 | **1** | **1** | 0 | **1** | 0 | **1** | 0 | **1** | E |
| compiles `a++`, `(?>a)`, `\Qa.b\E` | 0 | **1** | **1** | 0 | **1** | 0 | 0 | 0 | **1** | E |
| compiles `[[:alpha:]]` | 0 | **1** | **1** | **1** | **1** | **1** | 0 | 0 | **1** | **1** |
| compiles `(?i:a)b` (ES2025 modifiers) | 1 | 1 | 1 | **0** | 1 | **0** | 1 | 1 | 1 | **E** |

Bold marks a difference from V8. For `std::regex`, E on a compile row is a
thrown `regex_error`, that is a rejection. Two conclusions: `no_mod_m` and
`no_mod_s` are required for `^`, `$` and `.` to behave as ECMA-262 does
without the `m` and `s` flags, and the suite misses all three; and Boost
accepts much syntax ECMA-262 rejects, so a `format: "regex"` validator built
on Boost.Regex needs its own ECMA-262 syntax check anyway, which is most of a
regex parser. SRELL accepts `\Z` (and `\A`, `\z`): extensions added in
SRELL 2026.04 with no macro to turn them off.

Catastrophic backtracking, each case run alone with a 20 s limit:

| Case | V8 | Boost.Regex (all) | u32regex | SRELL | QuickJS (budget 2M backtracks) | PCRE2 | std::regex |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `(a*)*b` on 40 `a` | still running at 20 s | `error_complexity` thrown, 0.01 s | thrown, 0.35 s | `ecode() == error_complexity` (111), 0.02 s | `LRE_RET_TIMEOUT`, 0.01 s | no match, 0.00 s (required-character optimisation) | `error_complexity` thrown, 0.01 s |
| `(x+x+)+y` on 30 `x` | still running | thrown, 0.01 s | thrown, 0.40 s | 111, 0.02 s | timeout, 0.01 s | no match, 0.00 s | thrown |
| `^(\w+\s?)*$` on a 72-char sentence ending in `!` | still running | thrown, 0.01 s | thrown, 0.65 s | 111, 0.02 s | timeout, 0.01 s | `PCRE2_ERROR_MATCHLIMIT`, 0.08 s | thrown |

ECMA-262 has no limit, and V8 hangs, as JavaScript does: a validator of
untrusted schemas must bound matching itself.

## 3. Boost.Regex without exceptions

Built three ways, the same harness and `$S/noexcept_probe.cpp`: native with
`-fno-exceptions -DBOOST_NO_EXCEPTIONS`, wasip2 with the same flags (wasi-sdk
34, run by wasmtime), and wasip3 with wasm exceptions. Boost.Regex is
header-only and builds on all three; the suite results of every Boost
configuration are identical to the exceptions build on all three (checked
verdict by verdict). A minimal wasip2 program that compiles one pattern and
searches once is 3.6 MB, against 138 KB for an empty program: `cpp_regex_traits`
pulls in `std::locale` and iostreams.

What happens, by case (`boost::throw_exception` defined to print and
`std::abort()`, as `~/dev/webcpp/tools/throw_exception.cpp` does):

| Case | Native, exceptions | Native, `-fno-exceptions` | wasip2 (no exceptions) | wasip3 (wasm exceptions) |
| --- | --- | --- | --- | --- |
| Pattern does not compile, `no_except` flag | `status() == 8` (`error_paren`), `empty()` | the same | the same | the same |
| Pattern does not compile, no `no_except` | `regex_error` thrown | **`status() == 8`**: under `BOOST_NO_EXCEPTIONS`, `regex_error::raise()` is empty (`boost/regex/v5/pattern_except.hpp:62-70`), so a parse error is always a status | `status() == 8` | uncaught exception, trap |
| Searching with a regex that failed to compile | `std::invalid_argument` thrown | abort | abort (exit 134) | trap |
| Match reaches the complexity limit, `(a*)*b` or `(x+x+)+y` | `regex_error(error_complexity)` thrown, catchable | **abort** | **abort** (exit 134) | caught |

The complexity check is `perl_matcher_non_recursive.hpp:206` (recursion
depth over 80) and `:216` (`state_count > max_state_count`), both
`raise_error`, which is `raise_runtime_error`, which is
`boost::throw_exception` unconditionally (`pattern_except.hpp:83-98`).
`regex_constants::no_except` is a syntax option and governs only the
parser; there is no match flag, no `status()` on `match_results`, and no
non-throwing `regex_search`. `BOOST_REGEX_MAX_STATE_COUNT` (default
100,000,000) moves the limit; it cannot turn the overrun into a value. So:

- **Compiling** without exceptions works and gives an error value:

  ```cpp
  boost::regex re(pattern, boost::regex::perl | boost::regex::no_except);
  if (re.status() != 0) {
      return error(static_cast<boost::regex_constants::error_type>(re.status()));
  }
  ```

- **Matching** without exceptions has no recoverable form. A wasip2 build
  aborts the whole component on an adversarial `pattern` or instance; only a
  build with exceptions (native, or wasip3) can catch it. For a validator of
  untrusted input this disqualifies Boost.Regex as the default on the
  no-exceptions lane.

The working no-exception example is SRELL's (section 6): both the compile
error and the complexity overrun come back as values, on wasip2 without
exceptions:

```cpp
// -fno-exceptions -DSRELL_NO_THROW (or the per-object `quiet` flag)
srell::u8cregex re(pattern, srell::regex_constants::ECMAScript | srell::regex_constants::quiet);
if (re.ecode() != 0) return compile_error(re.ecode());      // e.g. 104
srell::u8ccmatch m;
bool found = srell::regex_search(first, last, m, re);
if (!found && m.ecode() != 0) return match_error(m.ecode()); // 111, error_complexity
return found;
```

Measured output on wasip2: `ecode=104` for `^(abc]`; `found=0
match.ecode=111` for `(a*)*b` on 40 `a`. SRELL has four `throw
std::bad_alloc()` (`srell.hpp` lines 1055, 1425, 1550, 1561) that
`SRELL_NO_THROW` does not guard, and which `-fno-exceptions` rejects at
compile time ("cannot use 'throw' with exceptions disabled", 20 errors);
`$S/srell-noexc/srell.hpp` replaces them with `std::abort()`, the same
policy as webcpp's `throw_exception`. The SRELL history says the no-bad_alloc
mode ("SRELL_NO_THROW 2") was "postponed indefinitely".

## 4. Boost.URL against the URI formats

`$S/url_harness.cpp` judges each string test of `optional/format/uri.json`,
`uri-reference.json`, `iri.json`, `iri-reference.json`, `uri-template.json`
(v1: `format/`, required there) with `boost::urls::parse_uri` (uri, iri) and
`parse_uri_reference` (uri-reference, iri-reference). Non-string tests are
not engine-dependent and are left out. Boost.URL has no RFC 6570 parser:
`boost::urls::format` uses `{}` replacement fields in the fmt style, a
different grammar, so `uri-template` is counted as unsupported.

| draft | format | total | passed | failed | unsupported | fail % |
|---|---|---|---|---|---|---|
| draft4 | uri | 41 | 41 | 0 | 0 | 0.0% |
| draft6 | uri | 41 | 41 | 0 | 0 | 0.0% |
| draft6 | uri-reference | 25 | 23 | 2 | 0 | 8.0% |
| draft6 | uri-template | 41 | 0 | 0 | 41 | 100.0% |
| draft7 | uri | 41 | 41 | 0 | 0 | 0.0% |
| draft7 | uri-reference | 25 | 23 | 2 | 0 | 8.0% |
| draft7 | iri | 20 | 15 | 5 | 0 | 25.0% |
| draft7 | iri-reference | 15 | 8 | 7 | 0 | 46.7% |
| draft7 | uri-template | 41 | 0 | 0 | 41 | 100.0% |
| draft2019-09 | uri | 41 | 41 | 0 | 0 | 0.0% |
| draft2019-09 | uri-reference | 25 | 23 | 2 | 0 | 8.0% |
| draft2019-09 | iri | 20 | 15 | 5 | 0 | 25.0% |
| draft2019-09 | iri-reference | 15 | 8 | 7 | 0 | 46.7% |
| draft2019-09 | uri-template | 41 | 0 | 0 | 41 | 100.0% |
| draft2020-12 | uri | 41 | 41 | 0 | 0 | 0.0% |
| draft2020-12 | uri-reference | 25 | 23 | 2 | 0 | 8.0% |
| draft2020-12 | iri | 20 | 15 | 5 | 0 | 25.0% |
| draft2020-12 | iri-reference | 15 | 8 | 7 | 0 | 46.7% |
| draft2020-12 | uri-template | 41 | 0 | 0 | 41 | 100.0% |
| v1 | uri | 41 | 41 | 0 | 0 | 0.0% |
| v1 | uri-reference | 25 | 23 | 2 | 0 | 8.0% |
| v1 | iri | 20 | 15 | 5 | 0 | 25.0% |
| v1 | iri-reference | 15 | 8 | 7 | 0 | 46.7% |
| v1 | uri-template | 41 | 0 | 0 | 41 | 100.0% |
| all | uri | 246 | 246 | 0 | 0 | 0.0% |
| all | uri-reference | 125 | 115 | 10 | 0 | 8.0% |
| all | iri | 80 | 60 | 20 | 0 | 25.0% |
| all | iri-reference | 60 | 32 | 28 | 0 | 46.7% |
| all | uri-template | 205 | 0 | 0 | 205 | 100.0% |
| all | uri + uri-reference | 371 | 361 | 10 | 0 | 2.7% |

Failing tests, deduplicated across drafts:

- uri-reference: "a non-numeric port in a network-path reference": `"//example.com:abc/p"` expected invalid, Boost.URL says valid [d6, d7, d2019-09, d2020-12, v1]
- uri-reference: "more than one at-sign in the authority": `"//a@b@example.com/"` expected invalid, Boost.URL says valid [d6, d7, d2019-09, d2020-12, v1]
- iri: "a valid IRI with anchor tag": `"http://\u0192\u00f8\u00f8.\u00df\u00e5r/?\u2202\u00e9\u0153=\u03c0\u00eex#\u03c0\u00ee\u00fcx"` expected valid, Boost.URL says invalid [d7, d2019-09, d2020-12, v1]
- iri: "a valid IRI with anchor tag and parentheses": `"http://\u0192\u00f8\u00f8.com/blah_(w\u00eek\u00efp\u00e9di\u00e5)_blah#\u00dfit\u00e9-1"` expected valid, Boost.URL says invalid [d7, d2019-09, d2020-12, v1]
- iri: "a valid IRI with URL-encoded stuff": `"http://\u0192\u00f8\u00f8.\u00df\u00e5r/?q=Test%20URL-encoded%20stuff"` expected valid, Boost.URL says invalid [d7, d2019-09, d2020-12, v1]
- iri: "a valid IRI with a supplementary-plane character in the path": `"http://\u0192\u00f8\u00f8.\u00df\u00e5r/\ud800\udf00"` expected valid, Boost.URL says invalid [d7, d2019-09, d2020-12, v1]
- iri: "a valid IRI with a supplementary-plane private-use char in query": `"http://\u0192\u00f8\u00f8.\u00df\u00e5r/?q=\udb80\udc00"` expected valid, Boost.URL says invalid [d7, d2019-09, d2020-12, v1]
- iri-reference: "a valid IRI": `"http://\u0192\u00f8\u00f8.\u00df\u00e5r/?\u2202\u00e9\u0153=\u03c0\u00eex#\u03c0\u00ee\u00fcx"` expected valid, Boost.URL says invalid [d7, d2019-09, d2020-12, v1]
- iri-reference: "a valid protocol-relative IRI Reference": `"//\u0192\u00f8\u00f8.\u00df\u00e5r/?\u2202\u00e9\u0153=\u03c0\u00eex#\u03c0\u00ee\u00fcx"` expected valid, Boost.URL says invalid [d7, d2019-09, d2020-12, v1]
- iri-reference: "a valid relative IRI Reference": `"/\u00e2\u03c0\u03c0"` expected valid, Boost.URL says invalid [d7, d2019-09, d2020-12, v1]
- iri-reference: "a valid IRI Reference": `"\u00e2\u03c0\u03c0"` expected valid, Boost.URL says invalid [d7, d2019-09, d2020-12, v1]
- iri-reference: "a valid IRI fragment": `"#\u0192r\u00e4gm\u00eant"` expected valid, Boost.URL says invalid [d7, d2019-09, d2020-12, v1]
- iri-reference: "a valid relative IRI Reference with a supplementary-plane character": `"/\u00e2\u03c0\u03c0/\ud800\udf00"` expected valid, Boost.URL says invalid [d7, d2019-09, d2020-12, v1]
- iri-reference: "a query-only IRI Reference with a supplementary private-use char": `"?q=\udb80\udc00"` expected valid, Boost.URL says invalid [d7, d2019-09, d2020-12, v1]

The two `uri-reference` failures are a Boost.URL bug, the same one the
research found: after an authority in a relative reference it accepts a
path that does not start with `/` (RFC 3986 `path-abempty`). Parsed,
`//example.com:abc/p` gives host `example.com`, empty port and path `abc/p`;
`//a@b@example.com/` gives userinfo `a`, host `b` and path `@example.com/`
(`$S/url_bug.cpp`). With a scheme (`http://example.com:abc/p`) both are
rejected. Worth an upstream issue. The IRI failures are every test with a
non-ASCII character: Boost.URL implements RFC 3986 only, not RFC 3987.

Against the 10% rule: `uri` + `uri-reference` 2.7%, under. With `iri` and
`iri-reference` 11.4% and with `uri-template` 36.7% (263/716), over, but
those formats are outside what Boost.URL claims to parse.

Build:

- **Not header-only.** `boost/url/src.hpp` in 1.92 is `#error "src.hpp is
  discontinued and should not be used. Please use the compiled library
  instead."`, and a program using `parse_uri` without the library fails to
  link (undefined `boost::urls` symbols). A unity translation unit that
  `#include`s the 19 `.cpp` files of `libs/url/src` (with
  `BOOST_URL_SOURCE` and `BOOST_URL_STATIC_LINK`) builds and links natively,
  which is a way to vendor it, but those sources are not in Homebrew's or
  any installed Boost: they come from the source tree.
- **wasip2** (`-fno-exceptions -DBOOST_NO_EXCEPTIONS`): the 19 sources
  compile with wasi-sdk 34 without a change; the harness links against them
  and gives the same results as native, test by test.
- **wasip3** (wasm exceptions): the same.

## 5. Boost.Locale on WASI

Boost.Locale 1.92 is compiled. Its Jamfile builds the shared sources plus a
backend (`icu`, `posix`, `std`, `winapi`) and refuses to build without iconv
or ICU ("Boost.Locale needs either iconv or ICU library to be built"). It
also needs Boost.Charconv (compiled, two sources) and uses
`boost/thread/mutex.hpp`. wasi-libc (musl) provides `iconv`. `$S/build_locale.sh`
compiles the sources the Jamfile would select, without ICU, in three
variants; `$S/locale_probe.cpp` creates `pt_BR.UTF-8` with a messages path
and domain, a `.mo` catalog made by `msgfmt` from a two-message `.po`,
and prints `translate`, a plural `translate`, `format`, `to_upper`, three
charset conversions and a date.

| Target | posix + std backends, iconv | std backend, iconv | std backend, no iconv |
| --- | --- | --- | --- |
| wasip2, `-fno-exceptions -DBOOST_NO_EXCEPTIONS` | **17 of 28 sources fail** | 13 of 23 fail | 12 of 23 fail |
| wasip2 with wasm exceptions (not the webcpp policy) | 28/28 compile, links, runs as wasip3 | 23/23 | 23/23 |
| wasip3 (wasm exceptions) | 28/28 compile, links, runs | 23/23, runs | 23/23 compile; the generator throws "Invalid or unsupported charset: UTF-8" |

Every wasip2 failure is "cannot use 'throw' with exceptions disabled" or
"cannot use 'try' with exceptions disabled": Boost.Locale has 61 `throw` and
14 `try` in the sources used here and 8 `throw` in its public headers, and
no use of `BOOST_NO_EXCEPTIONS` or `boost::throw_exception` anywhere. A user
translation unit that only includes `<boost/locale.hpp>` already fails under
`-fno-exceptions` (`boost/locale/date_time.hpp:431`), and
`encoding_utf.hpp:43`, `detail/any_string.hpp:62` fail when instantiated.
Making it build would mean patching dozens of sites.

On wasip3, `wasmtime run -W exceptions=y --dir locale`:

```
backends: posix std
name=pt_BR.UTF-8 utf8=1
translate: Olá Mundo
plural: 3 arquivos
format: Ann has 3 items costing 1.23457e+06
upper: ÉCOLE
conv latin1: café
conv cp1252: café
conv shift_jis: あ
date: 01/01/71
```

The same program natively, with Homebrew's ICU-backed library, prints
`1.234.567,5` and `31 de dez. de 1970`. So on WASI, messages (`translate`,
plurals from `.mo` files, which need a preopened directory), `format`,
case conversion and charset conversion work; number, currency and date
formatting fall back to the C locale, because wasi-libc has no locale data
and there is no ICU. Verdict: **compiles and runs on wasip3 (and on wasip2
only with exceptions); does not compile on wasip2 without exceptions.** For
JSON Schema it was only ever a candidate for normalisation (ICU backend only)
and offers no IDNA, so it is not needed by `webcpp::json_schema`.

## 6. Alternatives to vendor

Boost.Regex is over 10% on required plus optional in every draft and on
required alone for v1. Measured on the same harness:

| | SRELL 2026.07 | QuickJS-ng libregexp | PCRE2 10.47 |
| --- | --- | --- | --- |
| Licence | BSD-2-Clause | MIT | BSD-3-Clause (with a binary-distribution exception) |
| Language, form | C++ (C++11 features used when available), **header-only** (`srell.hpp` 11,752 lines plus two generated Unicode tables, or a 24,680-line single header) | C, three files to compile (`libregexp.c` 3,522 lines, `libunicode.c`, generated `libunicode-table.h`) plus `cutils.h`, and three hooks the embedder defines | C, compiled library, CMake or autotools |
| Dialect | ECMAScript by design: `u` semantics always on a UTF-8/16/32 `char` type, `v` flag (`vmode`), variable-width lookbehind, named groups, ES2025 modifiers `(?i:...)` | ECMA-262 itself (the engine of QuickJS-ng's `RegExp`, ES2025 including `v` and modifiers) | Perl; JavaScript only through options (`ALT_BSUX`, `EXTRA_ALT_BSUX`, `ASCII_BSD`/`BSW`) and a translation layer, as Blaze does (`blaze/vendor/core/src/core/regex/regex.cc`) |
| Unicode properties | yes: General_Category, Script, Script_Extensions, binary properties, Unicode 18.0 tables | yes, generated from the UCD by `unicode_gen.c` | yes, with `PCRE2_UCP`, but PCRE2 names (`\p{L}`; `\p{Letter}` and `\p{digit}` fail to compile) |
| Input | UTF-8 directly (`u8cregex` over `char`) | **UTF-16 or Latin-1 only**: UTF-8 must be transcoded per match | UTF-8 directly |
| Suite, all drafts (fail % req / req+opt) | 0.6% / 0.9% as shipped; **0% / 0%** with `SRELL_NO_UBMOD` (881/881) | **0% / 0%** (881/881) | 5.6% / 15.3% raw (`\p{Letter}`, `\p{digit}`, U+FEFF, `format: regex` syntax) |
| Remaining deviations seen | `(?i)` at the start accepted unless `SRELL_NO_UBMOD`; `\A`, `\Z`, `\z` accepted (no macro to remove them) | none seen | `.` matches U+2028; variable-length lookbehind limited to 255 code units; accepts perl syntax |
| Bad pattern without exceptions | `SRELL_NO_THROW` (global) or `quiet` (per object): `regex.ecode()` | `lre_compile` returns `NULL` and a message | `pcre2_compile` returns `NULL` and an error code |
| Runaway match without exceptions | counter of 2^21 (`lcounter_defnum_`), then `match_results::ecode() == error_complexity`; `SRELL_NO_LIMIT_COUNTER` removes it; no public setter for the limit was found | `lre_check_timeout(opaque)` hook every 10,000 backtracks returns non-zero: `LRE_RET_TIMEOUT`; budget per call through `opaque` | `pcre2_set_match_limit`, `depth_limit`, `heap_limit`: `PCRE2_ERROR_MATCHLIMIT` |
| `-fno-exceptions` | **needs a patch**: 4 unguarded `throw std::bad_alloc()` | C, nothing to do | C, nothing to do |
| wasip2 (no exceptions) | built and run here: 881/881, error values as native | built and run here: 881/881, `LRE_RET_TIMEOUT` as native | not built here (C99; Emscripten ports exist) |
| wasip3 | built and run here | not built (no reason it would differ) | not built |
| Minimal wasip2 program (one compile, one search) | 0.70 MB | 0.36 MB | - |

Others, not measured: **CTRE** compiles patterns at C++ compile time, so it
cannot take a schema's runtime `pattern`; it remains useful for fixed format
grammars. **RE2** (BSD-3) guarantees linear time but has no lookaround and no
backreferences, so it fails ECMA-262 by design, and it brings Abseil.
**Hyperscan/Vectorscan** is x86-centric, has no backreferences or
lookaround, and is not ECMA-262. **std::regex** was measured: 4.3% / 19.6%,
no `\p`, no named groups, no lookbehind, throws on everything.

## 7. Recommendations

- **Regex: vendor SRELL as the default engine** of the regex policy, behind
  the concept the research proposed, with `SRELL_NO_UBMOD` and, on the
  no-exceptions lanes, `SRELL_NO_THROW` plus the four-line `bad_alloc` patch
  recorded in the vendor README. It is header-only (fits a header-only
  library), passes every regex-dependent test of every draft as V8 does,
  matches UTF-8 directly, has Unicode 18 property tables, and returns error
  values for both bad patterns and runaway matches. To watch: `\A`, `\Z`,
  `\z` are accepted extensions, so the `format: "regex"` check should reject
  those three escapes itself until SRELL offers a switch (worth asking
  upstream, together with a `bad_alloc`-free mode and a per-call limit).
- **QuickJS-ng libregexp as the fallback**, if a C file and transcoding to
  UTF-16 are acceptable: the most exact ECMA-262 (it is a JavaScript
  engine's), the smallest wasm, and the cleanest budget hook.
- **Boost.Regex only as an opt-in policy** for users who already link it and
  build with exceptions; with `no_mod_m | no_mod_s`, and with a documented
  limitation that it is not ECMA-262 (bytes, no `\p`, perl syntax) and aborts
  on a complexity overrun without exceptions. `u32regex` + ICU + a
  translation layer gets within 0.5% on the suite but needs ICU on every
  target and still aborts; not worth it as a default.
- **URI formats: keep Boost.URL as the default** for `uri` and
  `uri-reference` (2.7%, two tests caused by one upstream bug, which a thin
  check can cover: after `//authority`, the rest must be empty or start with
  `/`, `?` or `#`). It is a compiled library: link `boost_url`, or build its
  19 sources (or a unity TU of them) with the library; they build for wasip2
  and wasip3 unchanged. Write `iri`/`iri-reference` (RFC 3987 `ucschar` and
  `iprivate` on top of the same grammar) and `uri-template` (RFC 6570) as
  small parsers of our own.
- **Boost.Locale: do not depend on it.** It does not compile for wasip2
  under the no-exceptions policy, and on wasip3 it formats only in the C
  locale. Nothing in JSON Schema needs it.

## 8. Commands

All from `$S`, with `unset CPATH CPLUS_INCLUDE_PATH C_INCLUDE_PATH`,
`B=$(brew --prefix boost)`, `I=$(brew --prefix icu4c)`,
`P=$(brew --prefix pcre2)`, `SDK=~/dev/webcpp/.local/wasi-sdk/bin`.
`$S/run.sh` runs all of them.

```sh
# inputs
curl -sL -o srell.zip https://www.akenotsuki.com/misc/srell/releases/srell2026_07.zip && unzip -q srell.zip -d srell
mkdir srell-noexc && sed 's/throw std::bad_alloc();/std::abort();/' srell/srell.hpp > srell-noexc/srell.hpp \
    && cp srell/srell_ucfdata2.h srell/srell_updata3.h srell-noexc/
git clone --depth 1 https://github.com/quickjs-ng/quickjs.git quickjs-ng
for l in url locale charconv; do git clone --depth 1 --branch boost-1.92.0 https://github.com/boostorg/$l.git boost-$l; done

# 1. regex, native, every engine
clang -O2 -c quickjs-ng/libregexp.c -o qjs_libregexp.o; clang -O2 -c quickjs-ng/libunicode.c -o qjs_libunicode.o
clang++ -std=c++20 -O2 -DSPIKE_ICU -DSPIKE_SRELL -DSPIKE_PCRE2 -DSPIKE_QJS -DSPIKE_STD -I$B/include -I$I/include -I$P/include \
    harness.cpp qjs_libregexp.o qjs_libunicode.o -o harness -L$I/lib -licuuc -licui18n -licudata -L$P/lib -lpcre2-8
clang++ -std=c++20 -O2 -DSPIKE_SRELL -DSRELL_NO_UBMOD -I$B/include harness.cpp -o harness_noubmod
python3 suite.py ./harness "boost-perl,boost-ecma,boost-ecma-nomod,boost-ecma-nomod-tr,u32-perl,u32-nomod,u32-nomod-tr,srell,srell-noubmod@./harness_noubmod,pcre2,qjs,std-ecma,v8-u@node harness.mjs" out_native.json
python3 report.py out_native.json <engines> > regex_report.md
python3 extra.py > extra_report.md

# 2. without exceptions
NX="-fno-exceptions -DBOOST_NO_EXCEPTIONS -DSRELL_NO_THROW -DSRELL_NO_UBMOD -DSPIKE_SRELL -DSPIKE_SRELL_NOEXC"
clang++ -std=c++20 -O2 $NX -DSPIKE_ICU -I$B/include -I$I/include harness.cpp -o harness_noexc -L$I/lib -licuuc -licui18n -licudata
clang++ -std=c++20 -O2 $NX -I$B/include noexcept_probe.cpp -o probe_noexc
python3 suite.py ./harness_noexc boost-ecma-nomod,boost-ecma-nomod-tr,u32-nomod,u32-nomod-tr,srell-noubmod out_noexc.json
$SDK/clang --target=wasm32-wasip2 -O2 -c quickjs-ng/libregexp.c -o qjs_libregexp_p2.o   # and libunicode.c
$SDK/clang++ --target=wasm32-wasip2 -fno-exceptions -DBOOST_NO_EXCEPTIONS -std=c++20 -O2 -DSPIKE_SRELL -DSPIKE_SRELL_NOEXC \
    -DSRELL_NO_THROW -DSRELL_NO_UBMOD -DSPIKE_QJS -I$B/include harness.cpp qjs_libregexp_p2.o qjs_libunicode_p2.o -o harness_p2.wasm
$SDK/clang++ --target=wasm32-wasip3 -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false -std=c++20 -O2 -DSPIKE_SRELL \
    -DSRELL_NO_UBMOD -I$B/include harness.cpp -o harness_p3.wasm -lunwind
python3 suite.py "wasmtime run harness_p2.wasm" boost-ecma-nomod,boost-ecma-nomod-tr,srell-noubmod,qjs out_p2.json
python3 suite.py "wasmtime run -W exceptions=y harness_p3.wasm" boost-ecma-nomod,boost-ecma-nomod-tr,srell-noubmod out_p3.json
wasmtime run probe_p2.wasm boost-complexity            # and the other cases of noexcept_probe.cpp

# 3. Boost.URL
clang++ -std=c++20 -O2 -I$B/include url_harness.cpp -o url_native -L$B/lib -lboost_url && python3 url_suite.py ./url_native
for f in $(find boost-url/src -name '*.cpp'); do $SDK/clang++ --target=wasm32-wasip2 -fno-exceptions -DBOOST_NO_EXCEPTIONS \
    -std=c++20 -O2 -DBOOST_URL_SOURCE -DBOOST_URL_STATIC_LINK=1 -I$B/include -Iboost-url/src -c $f -o ...; done   # and wasip3
python3 url_suite.py "wasmtime run url_p2.wasm"; python3 url_suite.py "wasmtime run -W exceptions=y url_p3.wasm"

# 4. Boost.Locale
./build_locale.sh p2|p3|p2exc posix-iconv|std-iconv|std-noiconv
$SDK/clang++ --target=wasm32-wasip3 -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false -std=c++20 -O2 \
    -DBOOST_LOCALE_STATIC_LINK=1 -I$B/include locale_probe.cpp libboost_locale_p3-posix-iconv.a -o locale_p3.wasm -lunwind
wasmtime run -W exceptions=y --dir locale locale_p3.wasm
```

Files: `$S/suite.py` (extraction, evaluator, judging), `$S/harness.cpp`
(every engine and the translation prototype), `$S/harness.mjs` (V8),
`$S/report.py`, `$S/extra.py`, `$S/noexcept_probe.cpp`,
`$S/url_harness.cpp`, `$S/url_suite.py`, `$S/url_bug.cpp`,
`$S/build_locale.sh`, `$S/locale_probe.cpp`, `$S/size/` (the minimal
programs), results in `$S/out_*.json`, `$S/regex_report.md`,
`$S/key_report.md`, `$S/extra_report.md`, `$S/url_report.md`, `$S/run.log`.

## Appendix A. Every failing test, per configuration

Each line is one test, deduplicated across drafts; the drafts it fails in follow it.

### `boost-ecma-nomod`: 44 distinct failing tests

- opt/ecmascript-regex.json: "ECMA 262 \D matches everything but ascii digits" / "NKO DIGIT ZERO matches (unlike e.g. Python)": pattern `^\D$`, data `"\u07c0"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \D matches everything but ascii digits" / "NKO DIGIT ZERO (as \u escape) matches": pattern `^\D$`, data `"\u07c0"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \W matches everything but ascii letters" / "latin-1 e-acute matches (unlike e.g. Python)": pattern `^\W$`, data `"\u00e9"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \s matches whitespace" / "latin-1 non-breaking-space matches": pattern `^\s$`, data `"\u00a0"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \s matches whitespace" / "zero-width whitespace matches": pattern `^\s$`, data `"\ufeff"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \s matches whitespace" / "paragraph separator matches (line terminator)": pattern `^\s$`, data `"\u2029"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \s matches whitespace" / "EM SPACE matches (Space_Separator)": pattern `^\s$`, data `"\u2003"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \S matches everything but whitespace" / "Non-whitespace matches": pattern `^\S$`, data `"\u2013"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "ascii character in json string": pattern `\p{Letter}cole`, data `"Les hivers de mon enfance etaient des saisons longues, long`, expected valid: **error** ('\\p{Letter}cole': does not compile: error_ctype) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "literal unicode character in json string": pattern `\p{Letter}cole`, data `"Les hivers de mon enfance \u00e9taient des saisons longues,`, expected valid: **error** ('\\p{Letter}cole': does not compile: error_ctype) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "unicode character in hex format in string": pattern `\p{Letter}cole`, data `"Les hivers de mon enfance \u00e9taient des saisons longues,`, expected valid: **error** ('\\p{Letter}cole': does not compile: error_ctype) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "unicode matching is case-sensitive": pattern `\p{Letter}cole`, data `"LES HIVERS DE MON ENFANCE \u00c9TAIENT DES SAISONS LONGUES,`, expected invalid: **error** ('\\p{Letter}cole': does not compile: error_ctype) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "pattern with non-ASCII digits" / "non-ascii digits (BENGALI DIGIT FOUR, BENGALI DIGIT TWO)": pattern `^\p{digit}+$`, data `"\u09ea\u09e8"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "ascii character in json string": pattern `\p{Letter}cole`, data `{"l'ecole": "pas de vraie vie"}`, expected valid: **error** ('\\p{Letter}cole': does not compile: error_ctype) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "literal unicode character in json string": pattern `\p{Letter}cole`, data `{"l'\u00e9cole": "pas de vraie vie"}`, expected valid: **error** ('\\p{Letter}cole': does not compile: error_ctype) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "unicode character in hex format in string": pattern `\p{Letter}cole`, data `{"l'\u00e9cole": "pas de vraie vie"}`, expected valid: **error** ('\\p{Letter}cole': does not compile: error_ctype) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "unicode matching is case-sensitive": pattern `\p{Letter}cole`, data `{"L'\u00c9COLE": "PAS DE VRAIE VIE"}`, expected invalid: **error** ('\\p{Letter}cole': does not compile: error_ctype) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patternProperties with non-ASCII digits" / "non-ascii digits (BENGALI DIGIT FOUR, BENGALI DIGIT TWO)": pattern `^\p{digit}+$`, data `{"\u09ea\u09e8": "khajit has wares if you have coin"}`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/non-bmp-regex.json: "Proper UTF-16 surrogate pair handling: pattern" / "matches empty": pattern `^🐲*$`, data `""`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/non-bmp-regex.json: "Proper UTF-16 surrogate pair handling: pattern" / "matches two": pattern `^🐲*$`, data `"\ud83d\udc32\ud83d\udc32"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/non-bmp-regex.json: "Proper UTF-16 surrogate pair handling: patternProperties" / "doesn't match two": pattern `^🐲*$`, data `{"\ud83d\udc32\ud83d\udc32": "hello"}`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/format/ecmascript-regex.json: "\a is not an ECMA 262 control escape" / "when used as a pattern": pattern `\a`, data `"\\a"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "Python-specific regular expression syntax is not valid ECMA 262" / "inline comment group (?#...) is not ECMA 262": pattern `(?#comment)a`, data `"(?#comment)a"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "a single global inline flag (?i)": pattern `(?i)abc`, data `"(?i)abc"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "multiple global inline flags (?ims)": pattern `(?ims)abc`, data `"(?ims)abc"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "ECMA 262 lookbehind is valid, including variable width" / "a variable-width lookbehind (ES2018)": pattern `(?<=a+)b`, data `"(?<=a+)b"`, expected valid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "ECMA 262 character classes and escapes" / "an empty character class is valid ECMA 262": pattern `[]`, data `"[]"`, expected valid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "ECMA 262 character classes and escapes" / "a negated empty character class is valid ECMA 262": pattern `[^]`, data `"[^]"`, expected valid: **failed** [d7, d2019-09, d2020-12]
- pattern.json: "pattern with Unicode property escape requires unicode mode" / "ASCII letters match": pattern `^\p{Letter}+$`, data `"Hello"`, expected valid: **error** ('^\\p{Letter}+$': does not compile: error_ctype) [d2020-12]
- pattern.json: "pattern with Unicode property escape requires unicode mode" / "Non-ASCII letters match": pattern `^\p{Letter}+$`, data `"\u03c0"`, expected valid: **error** ('^\\p{Letter}+$': does not compile: error_ctype) [d2020-12]
- pattern.json: "pattern with Unicode property escape requires unicode mode" / "Digits do not match": pattern `^\p{Letter}+$`, data `"123"`, expected invalid: **error** ('^\\p{Letter}+$': does not compile: error_ctype) [d2020-12]
- patternProperties.json: "patternProperties with Unicode property escape" / "Unicode letter property name matches": pattern `^\p{Letter}+$`, data `{"\u03c0": 1}`, expected valid: **error** ('^\\p{Letter}+$': does not compile: error_ctype) [d2020-12, v1]
- patternProperties.json: "patternProperties with Unicode property escape" / "Non-letter property name does not match pattern": pattern `^\p{Letter}+$`, data `{"123": 1}`, expected valid: **error** ('^\\p{Letter}+$': does not compile: error_ctype) [d2020-12, v1]
- format/ecmascript-regex.json: "\a is not an ECMA 262 control escape" / "when used as a pattern": pattern `\a`, data `"\\a"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "Python-specific regular expression syntax is not valid ECMA 262" / "inline comment group (?#...) is not ECMA 262": pattern `(?#comment)a`, data `"(?#comment)a"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "a single global inline flag (?i)": pattern `(?i)abc`, data `"(?i)abc"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "multiple global inline flags (?ims)": pattern `(?ims)abc`, data `"(?ims)abc"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "ECMA 262 lookbehind is valid, including variable width" / "a variable-width lookbehind (ES2018)": pattern `(?<=a+)b`, data `"(?<=a+)b"`, expected valid: **failed** [v1]
- format/ecmascript-regex.json: "ECMA 262 character classes and escapes" / "an empty character class is valid ECMA 262": pattern `[]`, data `"[]"`, expected valid: **failed** [v1]
- format/ecmascript-regex.json: "ECMA 262 character classes and escapes" / "a negated empty character class is valid ECMA 262": pattern `[^]`, data `"[^]"`, expected valid: **failed** [v1]
- pattern.json: "pattern with Unicode property escape" / "a valid case with a Unicode letter": pattern `^\p{Letter}+$`, data `"\u03c0"`, expected valid: **error** ('^\\p{Letter}+$': does not compile: error_ctype) [v1]
- pattern.json: "pattern with Unicode property escape" / "another valid case with multiple Unicode letters": pattern `^\p{Letter}+$`, data `"\u03c0\u03bb"`, expected valid: **error** ('^\\p{Letter}+$': does not compile: error_ctype) [v1]
- pattern.json: "pattern with Unicode property escape" / "an invalid case where the string contains digits": pattern `^\p{Letter}+$`, data `"123"`, expected invalid: **error** ('^\\p{Letter}+$': does not compile: error_ctype) [v1]
- patternProperties.json: "patternProperties with Unicode property escape" / "Unicode letter property name matches but value type is incorrect": pattern `^\p{Letter}+$`, data `{"\u03c0": "1"}`, expected invalid: **error** ('^\\p{Letter}+$': does not compile: error_ctype) [v1]

### `boost-ecma-nomod-tr`: 25 distinct failing tests

- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "ascii character in json string": pattern `\p{Letter}cole`, data `"Les hivers de mon enfance etaient des saisons longues, long`, expected valid: **error** ('\\p{Letter}cole': translation: \p{Letter} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "literal unicode character in json string": pattern `\p{Letter}cole`, data `"Les hivers de mon enfance \u00e9taient des saisons longues,`, expected valid: **error** ('\\p{Letter}cole': translation: \p{Letter} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "unicode character in hex format in string": pattern `\p{Letter}cole`, data `"Les hivers de mon enfance \u00e9taient des saisons longues,`, expected valid: **error** ('\\p{Letter}cole': translation: \p{Letter} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "unicode matching is case-sensitive": pattern `\p{Letter}cole`, data `"LES HIVERS DE MON ENFANCE \u00c9TAIENT DES SAISONS LONGUES,`, expected invalid: **error** ('\\p{Letter}cole': translation: \p{Letter} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "pattern with non-ASCII digits" / "ascii digits": pattern `^\p{digit}+$`, data `"42"`, expected valid: **error** ('^\\p{digit}+$': translation: \p{digit} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "pattern with non-ASCII digits" / "ascii non-digits": pattern `^\p{digit}+$`, data `"-%#"`, expected invalid: **error** ('^\\p{digit}+$': translation: \p{digit} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "pattern with non-ASCII digits" / "non-ascii digits (BENGALI DIGIT FOUR, BENGALI DIGIT TWO)": pattern `^\p{digit}+$`, data `"\u09ea\u09e8"`, expected valid: **error** ('^\\p{digit}+$': translation: \p{digit} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "ascii character in json string": pattern `\p{Letter}cole`, data `{"l'ecole": "pas de vraie vie"}`, expected valid: **error** ('\\p{Letter}cole': translation: \p{Letter} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "literal unicode character in json string": pattern `\p{Letter}cole`, data `{"l'\u00e9cole": "pas de vraie vie"}`, expected valid: **error** ('\\p{Letter}cole': translation: \p{Letter} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "unicode character in hex format in string": pattern `\p{Letter}cole`, data `{"l'\u00e9cole": "pas de vraie vie"}`, expected valid: **error** ('\\p{Letter}cole': translation: \p{Letter} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "unicode matching is case-sensitive": pattern `\p{Letter}cole`, data `{"L'\u00c9COLE": "PAS DE VRAIE VIE"}`, expected invalid: **error** ('\\p{Letter}cole': translation: \p{Letter} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patternProperties with non-ASCII digits" / "ascii digits": pattern `^\p{digit}+$`, data `{"42": "life, the universe, and everything"}`, expected valid: **error** ('^\\p{digit}+$': translation: \p{digit} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patternProperties with non-ASCII digits" / "ascii non-digits": pattern `^\p{digit}+$`, data `{"-%#": "spending the year dead for tax reasons"}`, expected invalid: **error** ('^\\p{digit}+$': translation: \p{digit} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patternProperties with non-ASCII digits" / "non-ascii digits (BENGALI DIGIT FOUR, BENGALI DIGIT TWO)": pattern `^\p{digit}+$`, data `{"\u09ea\u09e8": "khajit has wares if you have coin"}`, expected valid: **error** ('^\\p{digit}+$': translation: \p{digit} on UTF-8 bytes) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/format/ecmascript-regex.json: "ECMA 262 lookbehind is valid, including variable width" / "a variable-width lookbehind (ES2018)": pattern `(?<=a+)b`, data `"(?<=a+)b"`, expected valid: **failed** [d7, d2019-09, d2020-12]
- pattern.json: "pattern with Unicode property escape requires unicode mode" / "ASCII letters match": pattern `^\p{Letter}+$`, data `"Hello"`, expected valid: **error** ('^\\p{Letter}+$': translation: \p{Letter} on UTF-8 bytes) [d2020-12]
- pattern.json: "pattern with Unicode property escape requires unicode mode" / "Non-ASCII letters match": pattern `^\p{Letter}+$`, data `"\u03c0"`, expected valid: **error** ('^\\p{Letter}+$': translation: \p{Letter} on UTF-8 bytes) [d2020-12]
- pattern.json: "pattern with Unicode property escape requires unicode mode" / "Digits do not match": pattern `^\p{Letter}+$`, data `"123"`, expected invalid: **error** ('^\\p{Letter}+$': translation: \p{Letter} on UTF-8 bytes) [d2020-12]
- patternProperties.json: "patternProperties with Unicode property escape" / "Unicode letter property name matches": pattern `^\p{Letter}+$`, data `{"\u03c0": 1}`, expected valid: **error** ('^\\p{Letter}+$': translation: \p{Letter} on UTF-8 bytes) [d2020-12, v1]
- patternProperties.json: "patternProperties with Unicode property escape" / "Non-letter property name does not match pattern": pattern `^\p{Letter}+$`, data `{"123": 1}`, expected valid: **error** ('^\\p{Letter}+$': translation: \p{Letter} on UTF-8 bytes) [d2020-12, v1]
- format/ecmascript-regex.json: "ECMA 262 lookbehind is valid, including variable width" / "a variable-width lookbehind (ES2018)": pattern `(?<=a+)b`, data `"(?<=a+)b"`, expected valid: **failed** [v1]
- pattern.json: "pattern with Unicode property escape" / "a valid case with a Unicode letter": pattern `^\p{Letter}+$`, data `"\u03c0"`, expected valid: **error** ('^\\p{Letter}+$': translation: \p{Letter} on UTF-8 bytes) [v1]
- pattern.json: "pattern with Unicode property escape" / "another valid case with multiple Unicode letters": pattern `^\p{Letter}+$`, data `"\u03c0\u03bb"`, expected valid: **error** ('^\\p{Letter}+$': translation: \p{Letter} on UTF-8 bytes) [v1]
- pattern.json: "pattern with Unicode property escape" / "an invalid case where the string contains digits": pattern `^\p{Letter}+$`, data `"123"`, expected invalid: **error** ('^\\p{Letter}+$': translation: \p{Letter} on UTF-8 bytes) [v1]
- patternProperties.json: "patternProperties with Unicode property escape" / "Unicode letter property name matches but value type is incorrect": pattern `^\p{Letter}+$`, data `{"\u03c0": "1"}`, expected invalid: **error** ('^\\p{Letter}+$': translation: \p{Letter} on UTF-8 bytes) [v1]

### `u32-nomod`: 28 distinct failing tests

- opt/ecmascript-regex.json: "ECMA 262 \d matches ascii digits only" / "NKO DIGIT ZERO does not match (unlike e.g. Python)": pattern `^\d$`, data `"\u07c0"`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \d matches ascii digits only" / "NKO DIGIT ZERO (as \u escape) does not match": pattern `^\d$`, data `"\u07c0"`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \D matches everything but ascii digits" / "NKO DIGIT ZERO matches (unlike e.g. Python)": pattern `^\D$`, data `"\u07c0"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \D matches everything but ascii digits" / "NKO DIGIT ZERO (as \u escape) matches": pattern `^\D$`, data `"\u07c0"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \w matches ascii letters only" / "latin-1 e-acute does not match (unlike e.g. Python)": pattern `^\w$`, data `"\u00e9"`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \W matches everything but ascii letters" / "latin-1 e-acute matches (unlike e.g. Python)": pattern `^\W$`, data `"\u00e9"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \s matches whitespace" / "zero-width whitespace matches": pattern `^\s$`, data `"\ufeff"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \S matches everything but whitespace" / "zero-width whitespace does not match": pattern `^\S$`, data `"\ufeff"`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "\w in patterns matches [A-Za-z0-9_], not unicode letters" / "literal unicode character in json string": pattern `\wcole`, data `"Les hivers de mon enfance \u00e9taient des saisons longues,`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "\w in patterns matches [A-Za-z0-9_], not unicode letters" / "unicode character in hex format in string": pattern `\wcole`, data `"Les hivers de mon enfance \u00e9taient des saisons longues,`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "\d in pattern matches [0-9], not unicode digits" / "non-ascii digits (BENGALI DIGIT FOUR, BENGALI DIGIT TWO)": pattern `^\d+$`, data `"\u09ea\u09e8"`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "\w in patternProperties matches [A-Za-z0-9_], not unicode letters" / "literal unicode character in json string": pattern `\wcole`, data `{"l'\u00e9cole": "pas de vraie vie"}`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "\w in patternProperties matches [A-Za-z0-9_], not unicode letters" / "unicode character in hex format in string": pattern `\wcole`, data `{"l'\u00e9cole": "pas de vraie vie"}`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "\d in patternProperties matches [0-9], not unicode digits" / "non-ascii digits (BENGALI DIGIT FOUR, BENGALI DIGIT TWO)": pattern `^\d+$`, data `{"\u09ea\u09e8": "khajit has wares if you have coin"}`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/format/ecmascript-regex.json: "\a is not an ECMA 262 control escape" / "when used as a pattern": pattern `\a`, data `"\\a"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "Python-specific regular expression syntax is not valid ECMA 262" / "inline comment group (?#...) is not ECMA 262": pattern `(?#comment)a`, data `"(?#comment)a"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "a single global inline flag (?i)": pattern `(?i)abc`, data `"(?i)abc"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "multiple global inline flags (?ims)": pattern `(?ims)abc`, data `"(?ims)abc"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "ECMA 262 lookbehind is valid, including variable width" / "a variable-width lookbehind (ES2018)": pattern `(?<=a+)b`, data `"(?<=a+)b"`, expected valid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "ECMA 262 character classes and escapes" / "an empty character class is valid ECMA 262": pattern `[]`, data `"[]"`, expected valid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "ECMA 262 character classes and escapes" / "a negated empty character class is valid ECMA 262": pattern `[^]`, data `"[^]"`, expected valid: **failed** [d7, d2019-09, d2020-12]
- format/ecmascript-regex.json: "\a is not an ECMA 262 control escape" / "when used as a pattern": pattern `\a`, data `"\\a"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "Python-specific regular expression syntax is not valid ECMA 262" / "inline comment group (?#...) is not ECMA 262": pattern `(?#comment)a`, data `"(?#comment)a"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "a single global inline flag (?i)": pattern `(?i)abc`, data `"(?i)abc"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "multiple global inline flags (?ims)": pattern `(?ims)abc`, data `"(?ims)abc"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "ECMA 262 lookbehind is valid, including variable width" / "a variable-width lookbehind (ES2018)": pattern `(?<=a+)b`, data `"(?<=a+)b"`, expected valid: **failed** [v1]
- format/ecmascript-regex.json: "ECMA 262 character classes and escapes" / "an empty character class is valid ECMA 262": pattern `[]`, data `"[]"`, expected valid: **failed** [v1]
- format/ecmascript-regex.json: "ECMA 262 character classes and escapes" / "a negated empty character class is valid ECMA 262": pattern `[^]`, data `"[^]"`, expected valid: **failed** [v1]

### `u32-nomod-tr`: 2 distinct failing tests

- opt/format/ecmascript-regex.json: "ECMA 262 lookbehind is valid, including variable width" / "a variable-width lookbehind (ES2018)": pattern `(?<=a+)b`, data `"(?<=a+)b"`, expected valid: **failed** [d7, d2019-09, d2020-12]
- format/ecmascript-regex.json: "ECMA 262 lookbehind is valid, including variable width" / "a variable-width lookbehind (ES2018)": pattern `(?<=a+)b`, data `"(?<=a+)b"`, expected valid: **failed** [v1]

### `srell`: 4 distinct failing tests

- opt/format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "a single global inline flag (?i)": pattern `(?i)abc`, data `"(?i)abc"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "multiple global inline flags (?ims)": pattern `(?ims)abc`, data `"(?ims)abc"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "a single global inline flag (?i)": pattern `(?i)abc`, data `"(?i)abc"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "multiple global inline flags (?ims)": pattern `(?ims)abc`, data `"(?ims)abc"`, expected invalid: **failed** [v1]

### `srell-noubmod`: 0 distinct failing tests


### `qjs`: 0 distinct failing tests


### `pcre2`: 39 distinct failing tests

- opt/ecmascript-regex.json: "ECMA 262 \s matches whitespace" / "zero-width whitespace matches": pattern `^\s$`, data `"\ufeff"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \S matches everything but whitespace" / "zero-width whitespace does not match": pattern `^\S$`, data `"\ufeff"`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "ascii character in json string": pattern `\p{Letter}cole`, data `"Les hivers de mon enfance etaient des saisons longues, long`, expected valid: **error** ('\\p{Letter}cole': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "literal unicode character in json string": pattern `\p{Letter}cole`, data `"Les hivers de mon enfance \u00e9taient des saisons longues,`, expected valid: **error** ('\\p{Letter}cole': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "unicode character in hex format in string": pattern `\p{Letter}cole`, data `"Les hivers de mon enfance \u00e9taient des saisons longues,`, expected valid: **error** ('\\p{Letter}cole': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "unicode matching is case-sensitive": pattern `\p{Letter}cole`, data `"LES HIVERS DE MON ENFANCE \u00c9TAIENT DES SAISONS LONGUES,`, expected invalid: **error** ('\\p{Letter}cole': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "pattern with non-ASCII digits" / "ascii digits": pattern `^\p{digit}+$`, data `"42"`, expected valid: **error** ('^\\p{digit}+$': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "pattern with non-ASCII digits" / "ascii non-digits": pattern `^\p{digit}+$`, data `"-%#"`, expected invalid: **error** ('^\\p{digit}+$': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "pattern with non-ASCII digits" / "non-ascii digits (BENGALI DIGIT FOUR, BENGALI DIGIT TWO)": pattern `^\p{digit}+$`, data `"\u09ea\u09e8"`, expected valid: **error** ('^\\p{digit}+$': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "ascii character in json string": pattern `\p{Letter}cole`, data `{"l'ecole": "pas de vraie vie"}`, expected valid: **error** ('\\p{Letter}cole': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "literal unicode character in json string": pattern `\p{Letter}cole`, data `{"l'\u00e9cole": "pas de vraie vie"}`, expected valid: **error** ('\\p{Letter}cole': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "unicode character in hex format in string": pattern `\p{Letter}cole`, data `{"l'\u00e9cole": "pas de vraie vie"}`, expected valid: **error** ('\\p{Letter}cole': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "unicode matching is case-sensitive": pattern `\p{Letter}cole`, data `{"L'\u00c9COLE": "PAS DE VRAIE VIE"}`, expected invalid: **error** ('\\p{Letter}cole': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patternProperties with non-ASCII digits" / "ascii digits": pattern `^\p{digit}+$`, data `{"42": "life, the universe, and everything"}`, expected valid: **error** ('^\\p{digit}+$': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patternProperties with non-ASCII digits" / "ascii non-digits": pattern `^\p{digit}+$`, data `{"-%#": "spending the year dead for tax reasons"}`, expected invalid: **error** ('^\\p{digit}+$': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patternProperties with non-ASCII digits" / "non-ascii digits (BENGALI DIGIT FOUR, BENGALI DIGIT TWO)": pattern `^\p{digit}+$`, data `{"\u09ea\u09e8": "khajit has wares if you have coin"}`, expected valid: **error** ('^\\p{digit}+$': does not compile: unknown property after \P or \p) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/format/ecmascript-regex.json: "\a is not an ECMA 262 control escape" / "when used as a pattern": pattern `\a`, data `"\\a"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "Python-specific regular expression syntax is not valid ECMA 262" / "Python named group (?P<name>...) is not ECMA 262": pattern `(?P<name>x)`, data `"(?P<name>x)"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "Python-specific regular expression syntax is not valid ECMA 262" / "Python named backreference (?P=name) is not ECMA 262": pattern `(?P<n>a)(?P=n)`, data `"(?P<n>a)(?P=n)"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "Python-specific regular expression syntax is not valid ECMA 262" / "inline comment group (?#...) is not ECMA 262": pattern `(?#comment)a`, data `"(?#comment)a"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "a single global inline flag (?i)": pattern `(?i)abc`, data `"(?i)abc"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "multiple global inline flags (?ims)": pattern `(?ims)abc`, data `"(?ims)abc"`, expected invalid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "ECMA 262 lookbehind is valid, including variable width" / "a variable-width lookbehind (ES2018)": pattern `(?<=a+)b`, data `"(?<=a+)b"`, expected valid: **failed** [d7, d2019-09, d2020-12]
- pattern.json: "pattern with Unicode property escape requires unicode mode" / "ASCII letters match": pattern `^\p{Letter}+$`, data `"Hello"`, expected valid: **error** ('^\\p{Letter}+$': does not compile: unknown property after \P or \p) [d2020-12]
- pattern.json: "pattern with Unicode property escape requires unicode mode" / "Non-ASCII letters match": pattern `^\p{Letter}+$`, data `"\u03c0"`, expected valid: **error** ('^\\p{Letter}+$': does not compile: unknown property after \P or \p) [d2020-12]
- pattern.json: "pattern with Unicode property escape requires unicode mode" / "Digits do not match": pattern `^\p{Letter}+$`, data `"123"`, expected invalid: **error** ('^\\p{Letter}+$': does not compile: unknown property after \P or \p) [d2020-12]
- patternProperties.json: "patternProperties with Unicode property escape" / "Unicode letter property name matches": pattern `^\p{Letter}+$`, data `{"\u03c0": 1}`, expected valid: **error** ('^\\p{Letter}+$': does not compile: unknown property after \P or \p) [d2020-12, v1]
- patternProperties.json: "patternProperties with Unicode property escape" / "Non-letter property name does not match pattern": pattern `^\p{Letter}+$`, data `{"123": 1}`, expected valid: **error** ('^\\p{Letter}+$': does not compile: unknown property after \P or \p) [d2020-12, v1]
- format/ecmascript-regex.json: "\a is not an ECMA 262 control escape" / "when used as a pattern": pattern `\a`, data `"\\a"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "Python-specific regular expression syntax is not valid ECMA 262" / "Python named group (?P<name>...) is not ECMA 262": pattern `(?P<name>x)`, data `"(?P<name>x)"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "Python-specific regular expression syntax is not valid ECMA 262" / "Python named backreference (?P=name) is not ECMA 262": pattern `(?P<n>a)(?P=n)`, data `"(?P<n>a)(?P=n)"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "Python-specific regular expression syntax is not valid ECMA 262" / "inline comment group (?#...) is not ECMA 262": pattern `(?#comment)a`, data `"(?#comment)a"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "a single global inline flag (?i)": pattern `(?i)abc`, data `"(?i)abc"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "global inline flag groups are not valid ECMA 262" / "multiple global inline flags (?ims)": pattern `(?ims)abc`, data `"(?ims)abc"`, expected invalid: **failed** [v1]
- format/ecmascript-regex.json: "ECMA 262 lookbehind is valid, including variable width" / "a variable-width lookbehind (ES2018)": pattern `(?<=a+)b`, data `"(?<=a+)b"`, expected valid: **failed** [v1]
- pattern.json: "pattern with Unicode property escape" / "a valid case with a Unicode letter": pattern `^\p{Letter}+$`, data `"\u03c0"`, expected valid: **error** ('^\\p{Letter}+$': does not compile: unknown property after \P or \p) [v1]
- pattern.json: "pattern with Unicode property escape" / "another valid case with multiple Unicode letters": pattern `^\p{Letter}+$`, data `"\u03c0\u03bb"`, expected valid: **error** ('^\\p{Letter}+$': does not compile: unknown property after \P or \p) [v1]
- pattern.json: "pattern with Unicode property escape" / "an invalid case where the string contains digits": pattern `^\p{Letter}+$`, data `"123"`, expected invalid: **error** ('^\\p{Letter}+$': does not compile: unknown property after \P or \p) [v1]
- patternProperties.json: "patternProperties with Unicode property escape" / "Unicode letter property name matches but value type is incorrect": pattern `^\p{Letter}+$`, data `{"\u03c0": "1"}`, expected invalid: **error** ('^\\p{Letter}+$': does not compile: unknown property after \P or \p) [v1]

### `std-ecma`: 40 distinct failing tests

- opt/ecmascript-regex.json: "ECMA 262 \D matches everything but ascii digits" / "NKO DIGIT ZERO matches (unlike e.g. Python)": pattern `^\D$`, data `"\u07c0"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \D matches everything but ascii digits" / "NKO DIGIT ZERO (as \u escape) matches": pattern `^\D$`, data `"\u07c0"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \W matches everything but ascii letters" / "latin-1 e-acute matches (unlike e.g. Python)": pattern `^\W$`, data `"\u00e9"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \s matches whitespace" / "latin-1 non-breaking-space matches": pattern `^\s$`, data `"\u00a0"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \s matches whitespace" / "zero-width whitespace matches": pattern `^\s$`, data `"\ufeff"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \s matches whitespace" / "paragraph separator matches (line terminator)": pattern `^\s$`, data `"\u2029"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \s matches whitespace" / "EM SPACE matches (Space_Separator)": pattern `^\s$`, data `"\u2003"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "ECMA 262 \S matches everything but whitespace" / "Non-whitespace matches": pattern `^\S$`, data `"\u2013"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "ascii character in json string": pattern `\p{Letter}cole`, data `"Les hivers de mon enfance etaient des saisons longues, long`, expected valid: **error** ('\\p{Letter}cole': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "literal unicode character in json string": pattern `\p{Letter}cole`, data `"Les hivers de mon enfance \u00e9taient des saisons longues,`, expected valid: **error** ('\\p{Letter}cole': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "unicode character in hex format in string": pattern `\p{Letter}cole`, data `"Les hivers de mon enfance \u00e9taient des saisons longues,`, expected valid: **error** ('\\p{Letter}cole': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with pattern" / "unicode matching is case-sensitive": pattern `\p{Letter}cole`, data `"LES HIVERS DE MON ENFANCE \u00c9TAIENT DES SAISONS LONGUES,`, expected invalid: **error** ('\\p{Letter}cole': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "pattern with non-ASCII digits" / "ascii digits": pattern `^\p{digit}+$`, data `"42"`, expected valid: **error** ('^\\p{digit}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "pattern with non-ASCII digits" / "ascii non-digits": pattern `^\p{digit}+$`, data `"-%#"`, expected invalid: **error** ('^\\p{digit}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "pattern with non-ASCII digits" / "non-ascii digits (BENGALI DIGIT FOUR, BENGALI DIGIT TWO)": pattern `^\p{digit}+$`, data `"\u09ea\u09e8"`, expected valid: **error** ('^\\p{digit}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "ascii character in json string": pattern `\p{Letter}cole`, data `{"l'ecole": "pas de vraie vie"}`, expected valid: **error** ('\\p{Letter}cole': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "literal unicode character in json string": pattern `\p{Letter}cole`, data `{"l'\u00e9cole": "pas de vraie vie"}`, expected valid: **error** ('\\p{Letter}cole': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "unicode character in hex format in string": pattern `\p{Letter}cole`, data `{"l'\u00e9cole": "pas de vraie vie"}`, expected valid: **error** ('\\p{Letter}cole': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patterns always use unicode semantics with patternProperties" / "unicode matching is case-sensitive": pattern `\p{Letter}cole`, data `{"L'\u00c9COLE": "PAS DE VRAIE VIE"}`, expected invalid: **error** ('\\p{Letter}cole': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patternProperties with non-ASCII digits" / "ascii digits": pattern `^\p{digit}+$`, data `{"42": "life, the universe, and everything"}`, expected valid: **error** ('^\\p{digit}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patternProperties with non-ASCII digits" / "ascii non-digits": pattern `^\p{digit}+$`, data `{"-%#": "spending the year dead for tax reasons"}`, expected invalid: **error** ('^\\p{digit}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/ecmascript-regex.json: "patternProperties with non-ASCII digits" / "non-ascii digits (BENGALI DIGIT FOUR, BENGALI DIGIT TWO)": pattern `^\p{digit}+$`, data `{"\u09ea\u09e8": "khajit has wares if you have coin"}`, expected valid: **error** ('^\\p{digit}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/non-bmp-regex.json: "Proper UTF-16 surrogate pair handling: pattern" / "matches empty": pattern `^🐲*$`, data `""`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/non-bmp-regex.json: "Proper UTF-16 surrogate pair handling: pattern" / "matches two": pattern `^🐲*$`, data `"\ud83d\udc32\ud83d\udc32"`, expected valid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/non-bmp-regex.json: "Proper UTF-16 surrogate pair handling: patternProperties" / "doesn't match two": pattern `^🐲*$`, data `{"\ud83d\udc32\ud83d\udc32": "hello"}`, expected invalid: **failed** [d4, d6, d7, d2019-09, d2020-12, v1]
- opt/format/ecmascript-regex.json: "ECMA 262 named groups and backreferences are valid" / "an ECMA 262 named group (?<name>...)": pattern `(?<name>x)`, data `"(?<name>x)"`, expected valid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "ECMA 262 named groups and backreferences are valid" / "an ECMA 262 named backreference \k<name>": pattern `(?<n>a)\k<n>`, data `"(?<n>a)\\k<n>"`, expected valid: **failed** [d7, d2019-09, d2020-12]
- opt/format/ecmascript-regex.json: "ECMA 262 lookbehind is valid, including variable width" / "a variable-width lookbehind (ES2018)": pattern `(?<=a+)b`, data `"(?<=a+)b"`, expected valid: **failed** [d7, d2019-09, d2020-12]
- pattern.json: "pattern with Unicode property escape requires unicode mode" / "ASCII letters match": pattern `^\p{Letter}+$`, data `"Hello"`, expected valid: **error** ('^\\p{Letter}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [d2020-12]
- pattern.json: "pattern with Unicode property escape requires unicode mode" / "Non-ASCII letters match": pattern `^\p{Letter}+$`, data `"\u03c0"`, expected valid: **error** ('^\\p{Letter}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [d2020-12]
- pattern.json: "pattern with Unicode property escape requires unicode mode" / "Digits do not match": pattern `^\p{Letter}+$`, data `"123"`, expected invalid: **error** ('^\\p{Letter}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [d2020-12]
- patternProperties.json: "patternProperties with Unicode property escape" / "Unicode letter property name matches": pattern `^\p{Letter}+$`, data `{"\u03c0": 1}`, expected valid: **error** ('^\\p{Letter}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [d2020-12, v1]
- patternProperties.json: "patternProperties with Unicode property escape" / "Non-letter property name does not match pattern": pattern `^\p{Letter}+$`, data `{"123": 1}`, expected valid: **error** ('^\\p{Letter}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [d2020-12, v1]
- format/ecmascript-regex.json: "ECMA 262 named groups and backreferences are valid" / "an ECMA 262 named group (?<name>...)": pattern `(?<name>x)`, data `"(?<name>x)"`, expected valid: **failed** [v1]
- format/ecmascript-regex.json: "ECMA 262 named groups and backreferences are valid" / "an ECMA 262 named backreference \k<name>": pattern `(?<n>a)\k<n>`, data `"(?<n>a)\\k<n>"`, expected valid: **failed** [v1]
- format/ecmascript-regex.json: "ECMA 262 lookbehind is valid, including variable width" / "a variable-width lookbehind (ES2018)": pattern `(?<=a+)b`, data `"(?<=a+)b"`, expected valid: **failed** [v1]
- pattern.json: "pattern with Unicode property escape" / "a valid case with a Unicode letter": pattern `^\p{Letter}+$`, data `"\u03c0"`, expected valid: **error** ('^\\p{Letter}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [v1]
- pattern.json: "pattern with Unicode property escape" / "another valid case with multiple Unicode letters": pattern `^\p{Letter}+$`, data `"\u03c0\u03bb"`, expected valid: **error** ('^\\p{Letter}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [v1]
- pattern.json: "pattern with Unicode property escape" / "an invalid case where the string contains digits": pattern `^\p{Letter}+$`, data `"123"`, expected invalid: **error** ('^\\p{Letter}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [v1]
- patternProperties.json: "patternProperties with Unicode property escape" / "Unicode letter property name matches but value type is incorrect": pattern `^\p{Letter}+$`, data `{"\u03c0": "1"}`, expected invalid: **error** ('^\\p{Letter}+$': exception: The expression contained an invalid escaped character, or a trailing escape.) [v1]

