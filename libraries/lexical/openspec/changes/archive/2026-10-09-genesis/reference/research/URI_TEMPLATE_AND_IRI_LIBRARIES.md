---
title: "URI templates and IRIs in C++"
tags: [webcpp, json-schema, research]
status: active
created: 2026-10-08
---

# URI templates and IRIs in C++

Research input for `webcpp::json_schema`'s formats `uri-template` (RFC 6570)
and `iri` / `iri-reference` (RFC 3987). `uri` and `uri-reference` already use
Boost.URL. The question: is there an existing implementation to use before
writing our own, looking first in Boost, then in the C++ Alliance, then on
GitHub.

**Answer in one line:** Boost and the C++ Alliance have neither. On GitHub,
the only implementation that passes the whole suite is sourcemeta/core, which
is AGPL-3.0 or commercial and cannot be vendored. Everything else fails tests
or depends on exceptions. A hand-written validator passes 76/76 suite tests and
49/49 extra probes in about 170 lines of code, builds for wasip2 with
`-fno-exceptions` and allocates nothing. Recommendation: write it ourselves.

Scratch clones live in `~/.claude/jobs/fcf3a902/tmp/uri-research/`,
which is disposable. The prototype and the harnesses are kept next to this
note, in `~/.buzz/RESEARCH/URI_TEMPLATE_AND_IRI_PROBE/`.

## 1. What JSON Schema asks for

`json-schema-spec/specs/jsonschema-validation.md` (commit 4f56a99), lines
470-496:

- *iri*: "a valid IRI, according to RFC 3987"; *iri-reference*: "a valid IRI
  Reference (either an IRI or a relative-reference), according to RFC 3987".
- *uri-template*: "a valid URI Template (of any level), according to RFC 6570".
  The spec adds that "URI Templates may be used for IRIs; there is no separate
  IRI Template specification".

**IDNA is not required for `iri`.** RFC 3987's `ihost` is
`IP-literal / IPv4address / ireg-name`, and `ireg-name` is
`*( iunreserved / pct-encoded / sub-delims )`. That is grammar only: any
`ucschar` is allowed. RFC 3987 section 3.1 mentions IDNA ToASCII only for the
IRI-to-URI *mapping*, not for validity. The suite agrees: `http://ƒøø.ßår/`
must be valid, and no test depends on IDNA. Only `idn-hostname` and
`idn-email` need IDNA2008 (see `JSON_SCHEMA_FOR_WEBCPP.md`).

The ABNF does not say everything, though. RFC 3987 section 4.1 says "IRIs MUST
NOT contain bidirectional formatting characters" (U+200E, U+200F,
U+202A-U+202E), and these code points fall inside `ucschar`. The suite does not
test this. Section 4.2's structural bidi rules are a SHOULD and can be ignored.

**RFC 6570 has a verified erratum that the suite follows.** Erratum 6937
(verified 2022-05-06, https://www.rfc-editor.org/errata/eid6937) adds the
apostrophe `%x27` to `literals`: the original ABNF read
`%x26 / %x28-3B` and the corrected one reads `%x26-3B`. The suite's
`"a'b"` test expects valid. A further ambiguity: RFC 6570's ABNF includes the
`op-reserve` operators `= , ! @ |`, although section 2.2 reserves them for
future use. The suite only tests `{,+var}`, which is invalid either way.

**The test suite** (JSON-Schema-Test-Suite 5b0ee16, 2026-09-21): string tests
in `tests/v1/format/` are iri 20, iri-reference 15 and uri-template 41. The
draft7, 2019-09 and 2020-12 `optional/format/` files hold the same data and
expected results (checked with `jq`), and so does draft6's uri-template. Each
measurement below therefore counts 76 distinct cases. I added 49 extra probes
(`suite.py`, `EXTRA`) for corners the suite misses: reg-names that start like
IPv4 addresses, IPvFuture, zone ids, malformed UTF-8 (truncated, overlong,
surrogates), `iprivate` outside the query, U+0080 and U+FFFD, bidi formatting
characters, `{`, `}`, `{v*:3}`, `{a.}`, `{v:}`, and the forbidden literal
characters `< " ^ | \ \``.

## 2. Boost 1.92

Nothing in Boost does either job.

- **Headers.** I searched `/opt/homebrew/opt/boost/include/boost` (1_92) for
  `6570`, `3987`, the whole word `iri`, `uri template` and `uri_template`.
  The numeric matches are unrelated constants in math, decimal and hash2, and
  the other terms have no matches.
- **Boost.URL `format`** (`boost/url/format.hpp`) uses fmt-style replacement
  fields (`{}`, `{0}`, `{name}`, with fmt format specs), parsed in
  `src/detail/pattern.hpp`, whose comment says "parse uri templates or format
  strings". It is not RFC 6570: there are no operators (`+ # . / ; ? &`), no
  `:prefix` and no `*` explode, and its validity rules differ. It can neither
  validate nor expand RFC 6570 templates.
- **IRIs in Boost.URL.** None. Boost.URL rejects non-ASCII input:
  `parse_uri` and `parse_uri_reference` fail 5/20 iri and 7/15 iri-reference
  suite tests (spike harness `spike-regex/url_native`, rerun through this
  note's driver). Its open issues:
  - #441 "Review: explore IRIs" (2022-08-22, open). The maintainer
    (alandefreitas) wrote: "I don't think we should touch this until all else
    is stable", and suggested an `iri_view` type or an IRI-to-URI conversion
    built on Boost.Text.
  - #882 "parse_unicode_url function" (2024-11-11, open) proposes
    `parse_unicode_url` / `make_iri`, which would convert to a URI with
    punycode hosts and percent-encoding. It would convert, not validate. No
    code exists for it.
  - The branch `punycode` (last commit 2021-10-11 "Add punycode stuff",
    2 commits ahead and 726 behind `develop`) holds only
    `detail/punycode.hpp`. It is abandoned; issue #866 reports a bug in it.
  - Searching for "uri template", "RFC 6570", "IDNA" and "internationalized"
    in boostorg/url issues finds nothing more. A boostorg-wide code search for
    "RFC 6570", "RFC 3987", `iri_view` and `uri_template` finds only
    `pattern.hpp`.
- Beast and the other libraries have nothing either. The previous research
  ruled out Boost.Locale for Unicode or IDNA work.

## 3. The C++ Alliance (github.com/cppalliance)

There is no URI template or IRI repository and no proposal. I listed all
roughly 100 repositories and ran a code search for "uri template", 6570, IRI
and 3987. The search found only unrelated matches (PromptForge notes, math
tables, the word "IRIX").

- **cppalliance/punycode** (https://github.com/cppalliance/punycode, BSL-1.0
  in the file headers, no licence file, two commits on 2024-11-10 by Vinnie
  Falco, not maintained since). It has `boost::punycode` (header-only,
  373 lines) and `utf8_to_idna` (compiled, `src/idna.cpp`, 1,724 lines). Its
  IDNA is *Nameprep*, which is IDNA2003, not IDNA2008 or UTS #46. It is
  relevant only to Boost.URL #882's conversion idea, and `iri` does not need
  it.
- `burl`, `beast2`, `http`, `capy` and `corosio` (HTTP and I/O) contain no URI
  template or IRI code.

## 4. GitHub at large

### 4.1 RFC 6570 (URI Template)

A repository search for "uri template", "uritemplate", "uri-template" and
"rfc6570" in C++ and C finds only a few small projects:

| Candidate | Licence | Last commit / release | Header-only, std, deps | Validates? | Exceptions, alloc | Lines |
|---|---|---|---|---|---|---|
| **sourcemeta/core** `uritemplate` (https://github.com/sourcemeta/core) | **AGPL-3.0 or commercial** | daily (2026-10-08); no releases | compiled (CMake), C++20; core's `io`, `text`, `unicode`, `regex` (PCRE2) | yes: `URITemplate::is_uritemplate`, `noexcept`, plus expansion and a router | throws inside, catches in `is_uritemplate`; parser allocates tokens | 3,610 for the module (531 `helpers.h` and 136 `uritemplate.cc` are the grammar; the rest is the router) |
| Tinkoff/uri-template (https://github.com/Tinkoff/uri-template) | Apache-2.0 | 2022-02-18; v1.2.1 2021-08-27; 10 stars; inactive | compiled, C++17, no deps | parse, expand, reverse match | throws `std::runtime_error`; `std::string` / `shared_ptr` everywhere | 2,950 |
| returnzero23/uritemplate-cpp | MIT | 2020-12-05 | header-only, C++11 | **no**: expands only, accepts anything | allocates | 638 |
| fpelliccioni/cpp-uri-template | none | 2012-05-19 | C++11, Boost | targets draft 0.5, before the RFC | - | - |
| shiftinterrupt/URIT | LGPL-3.0 | 2016-12-05 | C | expansion | - | - |
| pulsecoreengineering/PulseCoAP | MIT | 2026 | ESP32 CoAP | CoAP-specific templates, not RFC 6570 | - | - |

The official RFC 6570 test corpus, https://github.com/uri-templates/uritemplate-test,
is for expansion. Its negative cases (`negative-tests.json`) are useful for
validation as well. sourcemeta vendors it.

### 4.2 RFC 3987 (IRI)

A repository search for "rfc3987", "IRI" and "internationalized resource
identifier" in C++ finds no dedicated library. Hits are in Python, Rust, PHP
and Perl. A code search for the ABNF rule names (`ucschar`, `iprivate`) in C++
finds the PEGTL grammar and its copies.

| Candidate | Licence | Activity | Header-only, std, deps | IRI per RFC 3987? | Exceptions, wasm | Lines |
|---|---|---|---|---|---|---|
| **sourcemeta/core** `uri` (`URI::is_iri`, `URI::is_iri_reference`, both `noexcept`) | **AGPL-3.0 or commercial** | daily | compiled, C++20; core's `io`, `ip`, `text`, `unicode` (generated UCD tables) | yes, a hand-written parser with `template <bool CheckOnly, bool IRI>` (`src/core/uri/parse.cc`) | throws and catches internally; wasm not tried | about 4,300 for the module (`parse.cc` 900, `uri.h` 1,503) |
| **taocpp/PEGTL** `tao/pegtl/example/iri.hpp` (https://github.com/taocpp/PEGTL) | BSL-1.0 on main and 4.x (3.x: MIT) | very active; 4.0.2 released 2026-09-21; 2.2k stars | header-only, C++20 on main (C++17 on 4.x); no deps | a PEG transcription of the ABNF, marked "experimental" in its own comment | **`#error "Exception support required"`** (`if_must` / `opt_must`). On main, wasi-sdk 34 also fails with `#error "Unknown operating system!"` in `pegtl/system.hpp` | 97 for the grammar (plus 97 for `uri.hpp`, plus ipv4/ipv6), over a 23,900-line library |
| Boost.URL | BSL-1.0 | active | compiled | no: ASCII only | - | - |
| ada-url/ada (https://github.com/ada-url/ada) | Apache-2.0 or MIT | v4.0.0 2026-07-27 | compiled, C++20 | **WHATWG URL**, not RFC 3987. No relative references without a base; lenient on `%`; IDNA (UTS #46) on hosts | essentially exception-free (3 `throw`s) | 32,300 |
| cpp-netlib/url "skyr-url" (https://github.com/cpp-netlib/url) | BSL-1.0 | last commit 2026-06-09; no releases | header-only, **C++23** (`std::expected`) | WHATWG, the same verdicts as ada | `SKYR_EXCEPTIONS_THROW` | 16,400 |
| uriparser (https://github.com/uriparser/uriparser) | BSD-3-Clause (LGPL-2.1+ for parts) | 1.0.2 2026-05-09 | C | RFC 3986 only. `URI_ENABLE_UNICODE` is a `wchar_t` API over the same ASCII grammar | - | - |
| drobilla/serd | ISC | active | C, RDF | its IRI reader is lenient, for Turtle; it is not a validator | - | - |

### 4.3 The C++ JSON Schema validators

| Validator | `uri-template` | `iri`, `iri-reference` | How |
|---|---|---|---|
| sourcemeta/blaze (AGPL-3.0 or commercial) | yes | yes | `evaluator_dispatch.h:966-973` calls core's `URITemplate::is_uritemplate`, `URI::is_iri` and `URI::is_iri_reference`. These are hand-written recursive-descent parsers, not regexes |
| jsoncons (BSL-1.0) | ignored | ignored | `keyword_validator_factory.hpp:385-445`: date-time, date, time, email, hostname, ipv4, ipv6, regex, json-pointer, uri and uri-reference. Any other name is "Not supported - ignore" |
| valijson (BSD-2-Clause) | ignored | ignored | `validation_visitor.hpp:413-496`: date, time, date-time and ipv4 only |
| pboettch/json-schema-validator (MIT) | throws | throws | `string-format-check.cpp:402-411` throws "not yet supported" for `iri`, `iri-reference` and `uri-template` |

## 5. Measurements

Driver: `suite.py` (adapted from `spike-regex/url_suite.py`). Each harness reads
`format \t hex` and answers 1, 0 or N. All harnesses were built natively with
Apple clang 21 and Boost 1.92, with `CPATH`, `CPLUS_INCLUDE_PATH` and
`C_INCLUDE_PATH` unset. The hand-written prototype was also built for wasip2
(wasi-sdk 34, `-fno-exceptions -DBOOST_NO_EXCEPTIONS`, Boost.URL's
`libboost_url_p2.a` from the spike) and run under wasmtime, with identical
results.

| Implementation | iri suite (20) | iri-ref suite (15) | uri-template suite (41) | iri extra (20) | iri-ref extra (7) | template extra (22) |
|---|---|---|---|---|---|---|
| sourcemeta/core | **20** | **15** | **41** | 19 | 7 | 22 |
| PEGTL `example/iri.hpp` (main) | 19 | 14 | - | 18 | 7 | - |
| Tinkoff/uri-template | - | - | 29 | - | - | 15 |
| Boost.URL `parse_uri[_reference]` | 15 | 8 | - | 16 | 4 | - |
| ada (`can_parse`) | 16 | - | - | 10 | - | - |
| skyr-url (`make_url`) | 16 | - | - | 10 | - | - |
| **hand-written prototype** | **20** | **15** | **41** | **20** | **7** | **22** |

The failures:

- **sourcemeta**: accepts U+200E (LRM), which RFC 3987 section 4.1 forbids.
  This is outside the ABNF and the suite does not test it.
- **PEGTL**:
  - It accepts `[::ffff:192.168.0.01]`, because `ipv4::octet` uses
    `from_chars`, which takes a leading zero.
  - It rejects `http://1.2.3.4.example.com/`. This is a PEG ordered-choice
    bug: `sor< IP_literal, IPv4address, ireg_name >` commits to IPv4 on the
    prefix `1.2.3.4` and never retries the input as a reg-name.
  - It accepts U+200E.
- **Tinkoff** (12 suite failures):
  - it does not check `%` triplets in literals or names (`a%`, `a%GG`, `{%}`);
  - it does not range-check the prefix (`{v:0}`, `{v:01}`, `{v:10000}`, `{v:}`);
  - it accepts `{a..b}`, `{a.}` and `{a,}`, and also a lone `{`;
  - it rejects `a'b`, because it predates erratum 6937;
  - it accepts any non-ASCII byte, including malformed UTF-8.
- **Boost.URL**: rejects every non-ASCII character. It also accepts an RFC 6874
  zone id (`[::1%25eth0]`), and, as the earlier spike found, `//a@b@c/` and
  `//a:abc/p` as URI references.
- **ada and skyr**: being WHATWG, they accept malformed percent-encoding, a
  trailing newline, U+0080, `iprivate` in the path and invalid UTF-8, and they
  reject IPvFuture.

## 6. The alternative: write it ourselves

The prototype is `URI_TEMPLATE_AND_IRI_PROBE/handwritten.hpp`: 182 lines, of
which 168 are code.

- **RFC 6570 validator: 54 lines.** A single pass over the input with no
  backtracking. Literals use the erratum-6937 ranges plus `ucschar` and
  `iprivate`. An expression is `{`, an optional operator, then varspecs
  separated by commas until `}`. A varname is a varchar (`ALPHA / DIGIT / _ /
  pct-encoded`) with single dots between varchars. After a varname comes an
  optional `:` with a max-length of `%x31-39` followed by at most 3 digits, or
  an optional `*`. The validator accepts `op-reserve` as the ABNF does. No
  expansion.
- **RFC 3987 validator: about 114 lines**, including:
  - strict UTF-8 decoding (RFC 3629: no truncation, overlongs, surrogates or
    values above U+10FFFF), 20 lines;
  - `ucschar` and `iprivate`, 15 lines;
  - the bidi formatting check from section 4.1;
  - per-component runs;
  - authority, scheme and port;
  - IPvFuture.

  Components are split as in RFC 3986 appendix B, at the first `#`, then the
  first `?`, then a scheme only when a `:` comes before any `/`, then `//`
  authority up to the next `/`. Each component is then checked against its
  own character set, so the PEG pitfalls above cannot happen. A colon before
  the first `/` that does not start a valid scheme is an error, which is
  exactly the `ipath-noscheme` rule. An `ireg-name` covers the IPv4 shape
  syntactically, so IPv4 needs no separate case.

  IPv6 literals are delegated to `boost::urls::parse_ipv6_address`, which was
  0/45 failures in the earlier spike and rejects zone ids and leading zeros. A
  self-contained IPv6 parser would add about 60 lines.
- **It has no exceptions and no allocation, and works on `std::string_view`.**
  It builds for wasip2 with `-fno-exceptions`.

For a production version, with doc comments, the spec page
(`doc/spec/...`) and error positions if the library reports them, I expect
roughly 150 lines for the template and 300 for IRI, 400 if IPv6 is written
in-house. Tests would be the suite files, the extra probes above, and the
negative cases from uritemplate-test.

The same IRI function with an `iri = false` template parameter (ASCII only,
no `ucschar`) would also validate `uri` and `uri-reference`. That would fix
the two Boost.URL `uri-reference` failures the earlier spike found
(`//example.com:abc/p` and `//a@b@example.com/`); sourcemeta does the same
with `template <bool IRI>`. One more option, not measured: map the IRI to a URI
(percent-encode each allowed non-ASCII code point) and call Boost.URL. It
allocates, and it would inherit Boost.URL's two reference bugs and its zone-id
acceptance.

## 7. Recommendation

Write both validators by hand inside `webcpp::json_schema`, one header each.
The RFC 6570 validator is about 150 lines. The RFC 3987 validator is about
300 lines, with a `bool IRI` parameter so that it also covers
`uri` / `uri-reference`. IPv6 comes from Boost.URL (or about 60 more lines in
house). No IDNA is needed.

Why not vendor:

- The only complete implementation, sourcemeta/core, is AGPL-3.0 or
  commercial.
- PEGTL's IRI grammar is BSL-1.0 and header-only, but it is labelled
  experimental, requires exceptions, does not build on wasi-sdk (main), fails
  2 suite tests and 2 probes, and would bring in a 24k-line parser library
  for 97 lines of grammar.
- Tinkoff fails 12/41 tests, throws and allocates, and has been inactive
  since 2022.
- WHATWG parsers (ada, skyr) implement a different standard.

sourcemeta/blaze's results, together with the suite and uritemplate-test,
remain a useful oracle for differential testing. Reading or running AGPL code
to compare results does not put its licence on our code; copying it would.

## Sources

- JSON-Schema-Test-Suite 5b0ee16: `~/.claude/jobs/fcf3a902/tmp/jsonschema-research/JSON-Schema-Test-Suite/tests/{v1/format,draft*/optional/format}/{iri,iri-reference,uri-template}.json`
- Spec: `~/.claude/jobs/fcf3a902/tmp/jsonschema-research/json-schema-spec/specs/jsonschema-validation.md` lines 470-496
- RFC 6570 https://www.rfc-editor.org/rfc/rfc6570 section 2; erratum 6937 https://www.rfc-editor.org/errata/eid6937
- RFC 3987 https://www.rfc-editor.org/rfc/rfc3987 sections 2.2, 3.1, 4.1
- Boost.URL issues https://github.com/boostorg/url/issues/441, /882, /866; branch https://github.com/boostorg/url/tree/punycode
- Clones (shallow, read-only) and builds: `~/.claude/jobs/fcf3a902/tmp/uri-research/` (`sm-build/` is sourcemeta core built with CMake with only uri, uritemplate and their dependencies)
- Harnesses, driver and prototype: `~/.buzz/RESEARCH/URI_TEMPLATE_AND_IRI_PROBE/` (`suite.py <binary>` prints the tables above)
- Previous research: `JSON_SCHEMA_FOR_WEBCPP.md`; `~/.claude/jobs/fcf3a902/tmp/jsonschema-research/sec_formats.md`; `~/.claude/jobs/fcf3a902/tmp/spike-regex/url_harness.cpp`
