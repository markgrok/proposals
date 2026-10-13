<!-- The specification of webcpp::formats as approved on 2026-10-09, kept as written. Since then: the library is named lexical (webcpp::lexical, <webcpp/lexical/...>, WEBCPP_LEXICAL_*); is_regex is its own allocation-free ECMA-262 syntax validator, so the sections on SRELL and webcpp::regex (5.12, 8.4) are history; the commit identity is R. ../specs/libraries/lexical/spec.md is the current contract. -->

---
title: "webcpp::formats: specification"
tags: [webcpp, formats, json-schema, boost-url, spec]
status: active
created: 2026-10-09
---

# webcpp::formats: specification

`webcpp::formats` is a library of webcpp's own (not a port): header-only
C++20 validators for the string formats JSON Schema names (`date-time`,
`email`, `hostname`, `ipv4`, `uri`, `iri`, `uri-template`, `json-pointer`,
`uuid`, `regex` and the rest), each a function that takes a
`std::string_view` and returns `bool`, allocating nothing, and each also a
public Boost.URL grammar rule that composes with Boost.URL's combinators.
ASCII grammars are written with Boost.URL's grammar library; the
internationalized formats use UTF-8-aware rules of formats' own that consult
`webcpp::idna`; `regex` is ECMA-262 through `webcpp::regex` (SRELL by
default), in a header of its own.

Its consumers are `webcpp::json_schema`, specified in parallel, whose default
format set names `formats::is_*`, and later `webcpp::zod`. The public
interface of section 4 is the contract those tracks code against. Where this
specification and the superproject's `AGENTS.md` differ, `AGENTS.md` wins;
where it and the owner's decisions differ, the owner's decisions win.

Sources: `JSON_SCHEMA_DESIGN.md` (decisions 11 and 12, sections 3, 8 and 11);
`~/.buzz/RESEARCH/URI_TEMPLATE_AND_IRI_LIBRARIES.md` and its prototype in
`URI_TEMPLATE_AND_IRI_PROBE/` (76/76 suite tests, 49/49 extra probes);
`~/.buzz/RESEARCH/JSON_SCHEMA_ENGINE_SPIKE.md` (SRELL; Boost.URL's
`uri-reference` bug and its guard); `~/.buzz/RESEARCH/JSON_SCHEMA_FOR_WEBCPP.md`
(the formats per draft, the suite's layout); Boost.URL 1.92's public grammar
headers; `WEBCPP_IDNA_SPEC.md`, the contract this library calls.

## 1. Purpose

| Format | Function | Rule (`formats::grammar`, or Boost.URL's) | Reference | Suite drafts |
| --- | --- | --- | --- | --- |
| `date-time` | `is_date_time` | `date_time_rule` | RFC 3339 5.6, 5.7 | 4, 6, 7, 2019-09, 2020-12, v1 |
| `date` | `is_date` | `date_rule` | RFC 3339 5.6 `full-date`, 5.7 | 7 on |
| `time` | `is_time` | `time_rule` | RFC 3339 5.6 `full-time`, 5.7 | 7 on (v1 refuses leap seconds) |
| `duration` | `is_duration` | `duration_rule` | RFC 3339 appendix A | 2019-09 on |
| `email` | `is_email` | `email_rule` | RFC 5321 4.1.2 `Mailbox`, 4.1.3, 4.5.3.1.1, 4.5.3.1.3 | all |
| `idn-email` | `is_idn_email` | `idn_email_rule` | RFC 6531 3.3 | 7 on |
| `hostname` | `is_hostname` | `hostname_rule` | RFC 1123 2.1; A-labels RFC 5891 4.4, RFC 5890 2.3.2.1 | all |
| `idn-hostname` | `is_idn_hostname` | `idn_hostname_rule`, `idna_label_rule` | RFC 5890 2.3.2.3, UTS 46 mapping (v1's registry) | 7 on |
| `ipv4` | `is_ipv4` | `boost::urls::ipv4_address_rule` | RFC 2673 3.2 | all |
| `ipv6` | `is_ipv6` | `boost::urls::ipv6_address_rule` | RFC 4291 2.2 | all |
| `uri` | `is_uri` | `boost::urls::uri_rule` | RFC 3986 3 | all |
| `uri-reference` | `is_uri_reference` | `uri_reference_rule` (Boost.URL's, guarded) | RFC 3986 4.1 | 6 on |
| `iri` | `is_iri` | `iri_rule` | RFC 3987 2.2, 3.1, 4.1 | 7 on |
| `iri-reference` | `is_iri_reference` | `iri_reference_rule` | RFC 3987 2.2, 3.1, 4.1 | 7 on |
| `uri-template` | `is_uri_template` | `uri_template_rule` | RFC 6570 2, erratum 6937 | 6 on |
| `json-pointer` | `is_json_pointer` | `json_pointer_rule` | RFC 6901 3 | 6 on |
| `relative-json-pointer` | `is_relative_json_pointer` | `relative_json_pointer_rule` | draft-handrews-relative-json-pointer-01 3 | 7 on |
| `uuid` | `is_uuid` | `uuid_rule` | RFC 9562 4 (RFC 4122 3) | 2019-09 on |
| `regex` | `is_regex` (`<webcpp/formats/regex.hpp>`) | none (section 5.12) | ECMA-262 `Pattern`, `u` mode, through `webcpp::regex` (SRELL by default) | 7 on |

Drafts 7 and 2019-09 cite draft-handrews-relative-json-pointer-01, which
has no index manipulation. Draft 2020-12's reference names
draft-handrews-relative-json-pointer-01 but dates it December 2020 with
B. Hutton as editor, which is draft-bhutton-relative-json-pointer-00's
metadata, and json-schema.org's specification links list bhutton-00 for
2020-12; v1 follows 2020-12. So index manipulation is a policy, allowed for
2020-12 and v1 and refused for 7 and 2019-09 (section 5.4, amended
2026-10-09; checked on the IETF archive the same day; for the owner to
confirm).

Not in scope: v1's RFC 9557 `date-time-ext` suffixes and its "published leap
seconds" (section 13, risk 8); the formats of v1's registry beyond the body's
17 (`int32`, `base64url` and the others); content (`contentEncoding`,
`contentMediaType`), which is json_schema's.

## 2. Decisions

### 2.1 The owner's (binding)

From the owner's approval of 2026-10-09 (`JSON_SCHEMA_DESIGN.md` section 8)
and the decisions it builds on:

1. **A library of its own**, `webcpp::formats`, repository
   `webcpporg/formats` (created with the owner's go).
2. **`string_view` in, `bool` out, nothing allocated**: `is_hostname`,
   `is_idn_hostname`, `is_email`, `is_idn_email`, `is_ipv4`, `is_ipv6`,
   `is_date_time`, `is_date`, `is_time`, `is_duration`, `is_uuid`, `is_uri`,
   `is_uri_reference`, `is_iri`, `is_iri_reference`, `is_uri_template`,
   `is_json_pointer`, `is_relative_json_pointer`.
3. **The same rules are public in `formats::grammar`** as Boost.URL rule
   objects, composable with Boost.URL's combinators.
4. **`<webcpp/formats/regex.hpp>`** (`is_regex`, ECMA-262 through
   `webcpp::regex`) is a separate header, the only one that includes
   `<webcpp/regex.hpp>`; formats never names SRELL in its build or its
   includes. (Superseded on 2026-10-10 by WEBCPP_REGEX_SPEC.md section 13.1:
   it read "only it depends on SRELL", with SRELL from the mirror as a
   submodule.)
5. **ASCII grammars are written with Boost.URL's grammar library**, reusing
   `ipv4_address_rule` (it refuses leading zeros) and `ipv6_address_rule`
   (also email's `[IPv6:...]`).
6. **`uri` uses `parse_uri`. `uri-reference` uses `parse_uri_reference` plus
   the three-line guard**; the fix is proposed upstream to `boostorg/url`, and
   the guard leaves when a Boost release carries it (decision 12).
7. **IRIs follow RFC 3987 section 3.1**: each non-ASCII code point is checked
   against `ucschar` (and `iprivate` in the query only), the IRI is mapped to a
   URI, and the URI is handed to Boost.URL. **`uri-template`** is a small RFC
   6570 grammar on the combinators, its literals reusing the IRI range check.
8. **IDN formats use custom UTF-8-aware rules plugged into Boost.URL's
   combinators, consulting `webcpp::idna`. Boost.URL is never modified.**
9. **Dependencies:** `webcpp::idna`, Boost.URL, and `webcpp::regex` for the
   regex header only.
10. **Exceptions:** `WEBCPP_FORMATS_NO_EXCEPTIONS` in `config.hpp`, as AGENTS
    chapter 6 says; every throw through `boost::throw_exception`; a throwing
    API behind the macro. formats raises nothing of its own.
11. **Allocation:** pmr wherever anything allocates; ideally nothing does.
12. **Targets:** native, wasip2, wasip3, emscripten.
13. **Tests:** the official JSON-Schema-Test-Suite's `optional/format/` cases
    (and v1's `format/`), run directly against these functions, plus unit
    tests and RFC examples. Target 100%; a residue tied to a Unicode version
    is a tracked known failure; CI fails on a new failure and on an
    unexpected pass.
14. **Documentation:** a page with complete Doc Comments and a strict MrDocs
    reference. **Changelog:** a line in "Unreleased" with every change.
    **Licence:** BSL-1.0, "Copyright (c) 2026 WebCpp.org"; the banned word
    nowhere; English; no em dash.
15. **Size:** about 700 lines of library code.

### 2.2 Proposed by this specification (for the owner's review)

- **P1. A rule yields the text it matched**, as `boost::core::string_view`
  (Boost.URL's text type, which converts to `std::string_view`). The one
  exception, `uri_reference_rule`, wraps Boost.URL's rule and yields what it
  yields, a `boost::urls::url_view`. Parsed values (a date's fields) are out
  of scope; adding them later is new rules, not a change of these.
- **P2. Rule types are public**, `<name>_rule_t`, each with a `constexpr`
  object `<name>_rule`, so that a parameterized rule (`time_rule_t{...}`,
  `iri_rule_t{&resource}`) is spelled without a factory function. Boost.URL
  hides its types in `implementation_defined`; formats documents its own, as
  a contract.
- **P3. No error category of formats' own.** The functions return `bool`; a
  rule reports Boost.URL's `grammar::error::mismatch` when the input does not
  start with its production (so a `variant_rule` tries the next), or
  `grammar::error::invalid` when it started and broke a constraint, and an
  IDN rule reports the `idna::error` that refused it.
- **P4. `is_time` takes a `leap_seconds` policy**, `allowed` by default
  (RFC 3339, drafts 7 to 2020-12), `refused` for v1, whose suite inverts six
  leap-second tests of `time.json` ("This MUST NOT include leap seconds").
  `date-time` allows them in every draft, v1 included, as its suite says.
- **P5. The IRI's mapped URI** needs contiguous storage, since Boost.URL
  parses a `string_view`. An ASCII IRI is handed to Boost.URL as it is,
  never copied; a non-ASCII one is mapped into an inline buffer of
  `detail::iri_inline_capacity` (1,024 octets: any IRI of up to 341 octets,
  since mapping at most triples a length); a longer one takes its buffer from
  a memory resource: the one given to the overload that takes one, else
  `std::pmr::get_default_resource()`. json_schema passes its validation's
  resource, as its decision 12 planned.
- **P6. `is_regex` compiles through `webcpp::regex`.** `is_regex(pattern)` is
  `regex::regex::compile(pattern, {.syntax = ECMAScript}).has_value()` with
  the default resource; `is_regex(pattern, resource)` gives `.memory =
  &resource` and allocates nothing outside it where `regex::honours<>::memory`
  holds (the default, SRELL); with another backend it allocates as that
  backend does, which the page says.
- **P7. One b2 target**, `formats`, whose usage requirements add
  `/webcpp/regex//regex` beside idna and Boost.URL (AGENTS chapter 2: one
  target named after the library). The separation decision 4 asks for is by
  header: only `regex.hpp` includes `webcpp/regex`, and no header includes
  `srell`, which the include boundaries check, and a program that never
  includes it pays nothing.
- **P8. The suite is read at run time** from the shared submodule, by the
  test alone, with Boost.JSON (`/webcpp//boost_json` in the test's
  requirements). No copy of the suite's data is committed: it is MIT data,
  and AGENTS keeps every file of webcpp BSL-1.0. On native and emscripten
  (`-sNODERAWFS=1`) the files are read today; on wasip2 and wasip3 the run
  needs S5, json_schema's mechanism for the same files.
- **P9. A hostname's A-label is judged as idn-hostname judges it**: each
  label starting with `xn--` (any case) is lowercased into a stack buffer and
  passed to `idna::check_label`, so `XN--NXASMQ6B` is valid in both formats.
  A label that is not an A-label follows RFC 1123 alone, so `ab--cd.example`
  is a valid hostname (draft 4's suite says so) and an invalid idn-hostname
  (RFC 5890's R-LDH labels are not NR-LDH).
- **P10. Email's domain is `hostname_rule`**, A-labels included; its address
  literals are IPv4 and `IPv6:` only. RFC 5321's General-address-literal
  needs a standardized tag registered with IANA, and `IPv6` is the only one,
  so `[RFC-5322-domain-literal]` is invalid, as the suite says.
- **P11. Zone identifiers.** `parse_uri` accepts an RFC 6874 zone id
  (`http://[::1%25eth0]/`); the suite does not test it. RFC 9844 (2025)
  obsoletes RFC 6874 and states that this "reverts the change that it made
  to the URI syntax defined by RFC 3986", and the owner ruled formats
  faithful to RFC 3986 (2026-10-09), so formats refuses a zone id through a
  guard of its own (`refuses_zone_identifier`), pins Boost.URL's acceptance
  in `uri_test` so a Boost that changes is seen, and records it among the
  page's known deviations from Boost.URL (amended 2026-10-09, formats Task
  7b).
- **P12. A UTF-8 decoder of formats' own** (`formats::detail`, about 20
  lines, RFC 3629 strict), rather than reaching into `idna::detail`: no
  library uses another's `detail`.

## 3. Layout and files

### 3.1 Where the work happens

The worktree of WEBCPP_IDNA_SPEC section 3.1: `~/dev/webcpp-formats`, branch
`lib/formats`, with `libs/idna` and `libs/formats` as local repositories
until the owner creates `webcpporg/formats` and adds the submodule. Commits
as `R <github.overboard312@passmail.net>`, no trailer, explicit paths. The
superproject changes formats needs (S2 to S5, section 8.3) are shared with
json_schema and are built once, on a branch of their own, by whichever track
reaches them first.

### 3.2 The tree

```
libs/formats/
  build.jam                                project /webcpp/formats, alias formats
  meta/libraries.json  meta/include-boundaries.json
  include/webcpp/formats.hpp               every public header but regex.hpp (webcpp::regex)
  include/webcpp/formats/config.hpp        WEBCPP_FORMATS_NO_EXCEPTIONS
  include/webcpp/formats/date_time.hpp     date-time, date, time, duration; leap_seconds
  include/webcpp/formats/uuid.hpp
  include/webcpp/formats/json_pointer.hpp  json-pointer, relative-json-pointer
  include/webcpp/formats/ip.hpp            ipv4, ipv6
  include/webcpp/formats/uri.hpp           uri, uri-reference, iri, iri-reference
  include/webcpp/formats/uri_template.hpp
  include/webcpp/formats/hostname.hpp      hostname, idn-hostname, idna_label_rule
  include/webcpp/formats/email.hpp         email, idn-email
  include/webcpp/formats/regex.hpp         regex, the only header that includes webcpp::regex
  include/webcpp/formats/detail/utf8.hpp          strict decoding (P12)
  include/webcpp/formats/detail/charsets.hpp      lut_chars of each grammar; ucschar, iprivate
  include/webcpp/formats/detail/rules.hpp         small terminal rules: exactly<N>, two_digits
  include/webcpp/formats/detail/uri_guard.hpp     the uri-reference guard
  include/webcpp/formats/detail/iri_mapping.hpp   RFC 3987 3.1, inline buffer, resource
  include/webcpp/formats/detail/idn_scan.hpp      where an IDN run ends
  test/Jamfile  test/*_test.cpp  test/require.hpp  test/.clang-tidy (if measured necessary)
  test/known_failures/<dialect>.json       draft4 ... v1, expected empty
  example/Jamfile  example/formats_*.cpp  example/formats_*.expected
  doc/Jamfile  doc/mrdocs.yml  doc/formats.adoc (+ section files)  doc/counts.py
  README.md  AGENTS.md  LICENSE_1_0.txt  .gitignore  .gitattributes
  .github/workflows/ci.yml                 library.yml with library: formats
```

formats' `AGENTS.md` holds what is specific: the guard and its upstream
issue, webcpp::regex and its configuration, why IRIs are mapped, the suite
submodule and the known-failure lists.

### 3.3 `build.jam`, `meta/`

```
project /webcpp/formats ;

alias formats
  : : : : <include>include <library>/boost//headers
    <library>/webcpp/idna//idna
    <library>/webcpp//boost_url
    <library>/webcpp/regex//regex ;
```

- `meta/libraries.json`: key and name `formats`, authors `["R"]`,
  description "Validators and Boost.URL grammar rules for the string formats
  of JSON Schema, for C++20.", category `["Parsing", "String"]`,
  `"cxxstd": "20"`, `"port-of": null`.
- `meta/include-boundaries.json`: every header must not include `srell`
  ("formats reaches SRELL only through webcpp::regex"); every header but
  `regex.hpp` must not include `webcpp/regex` ("only regex.hpp depends on
  webcpp::regex"); every header must not include `webcpp/idna/detail`
  ("formats uses idna's public interface") or `boost/json` ("JSON is the
  suite runner's, not the library's").

## 4. Public API (the contract)

All of it is in `namespace webcpp::formats` and `webcpp::formats::grammar`.
Every function and rule is frozen once its increment lands. `detail` is named
for orientation only.

### 4.1 The validators

```cpp
namespace webcpp::formats {

// <webcpp/formats/date_time.hpp>
enum class leap_seconds { allowed, refused };   // P4

[[nodiscard]] bool is_date_time(std::string_view s) noexcept;
[[nodiscard]] bool is_date(std::string_view s) noexcept;
[[nodiscard]] bool is_time(std::string_view s, leap_seconds leap = leap_seconds::allowed) noexcept;
[[nodiscard]] bool is_duration(std::string_view s) noexcept;

// <webcpp/formats/uuid.hpp>
[[nodiscard]] bool is_uuid(std::string_view s) noexcept;

// <webcpp/formats/json_pointer.hpp>
[[nodiscard]] bool is_json_pointer(std::string_view s) noexcept;
enum class index_manipulation { allowed, refused };
[[nodiscard]] bool is_relative_json_pointer(std::string_view s,
                                            index_manipulation p = index_manipulation::allowed) noexcept;

// <webcpp/formats/ip.hpp>
[[nodiscard]] bool is_ipv4(std::string_view s) noexcept;
[[nodiscard]] bool is_ipv6(std::string_view s) noexcept;

// <webcpp/formats/uri.hpp>
[[nodiscard]] bool is_uri(std::string_view s) noexcept;
[[nodiscard]] bool is_uri_reference(std::string_view s) noexcept;
[[nodiscard]] bool is_iri(std::string_view s);                       // P5: may allocate beyond 1,024 octets
[[nodiscard]] bool is_iri(std::string_view s, std::pmr::memory_resource& resource);
[[nodiscard]] bool is_iri_reference(std::string_view s);
[[nodiscard]] bool is_iri_reference(std::string_view s, std::pmr::memory_resource& resource);

// <webcpp/formats/uri_template.hpp>
[[nodiscard]] bool is_uri_template(std::string_view s) noexcept;

// <webcpp/formats/hostname.hpp>
[[nodiscard]] bool is_hostname(std::string_view s) noexcept;
[[nodiscard]] bool is_idn_hostname(std::string_view s) noexcept;

// <webcpp/formats/email.hpp>
[[nodiscard]] bool is_email(std::string_view s) noexcept;
[[nodiscard]] bool is_idn_email(std::string_view s) noexcept;

}  // namespace webcpp::formats

// <webcpp/formats/regex.hpp>, the only header that includes <webcpp/regex.hpp>
namespace webcpp::formats {
[[nodiscard]] bool is_regex(std::string_view pattern, std::pmr::memory_resource& resource);
[[nodiscard]] bool is_regex(std::string_view pattern);   // the default resource
}
```

Every function takes any byte string: one that is not well-formed UTF-8 is
invalid in every format (and in the ASCII formats, any byte above 0x7F is).
Each `is_x(s)` is `grammar::parse(s, grammar::x_rule).has_value()`, so the
function and its rule never disagree; `is_ipv4`, `is_ipv6` and `is_uri` use
Boost.URL's rules and `parse_uri`.

What json_schema relies on: every validator is callable as
`bool(std::string_view)` (the defaulted parameters included), and
`is_time(s, formats::leap_seconds::refused)` is v1's `time`; for IRIs with
its validation resource, `is_iri(s, resource)` and
`is_iri_reference(s, resource)`.

### 4.2 The rules

```cpp
namespace webcpp::formats::grammar {

using text = boost::core::string_view;          // what every rule of formats yields (P1)
template <class T>
using result = boost::system::result<T>;

// Each type models Boost.URL's Rule: value_type, a const parse(char const*& it,
// char const* end), nothrow copy. On success `it` is past the match; on failure
// it is unchanged.

struct date_rule_t      { using value_type = text; result<text> parse(char const*&, char const*) const noexcept; };
struct time_rule_t      { using value_type = text; leap_seconds leap = leap_seconds::allowed;
                          result<text> parse(char const*&, char const*) const noexcept; };
struct date_time_rule_t { using value_type = text; result<text> parse(char const*&, char const*) const noexcept; };
struct duration_rule_t  { using value_type = text; result<text> parse(char const*&, char const*) const noexcept; };
struct uuid_rule_t      { using value_type = text; result<text> parse(char const*&, char const*) const noexcept; };
struct json_pointer_rule_t          { using value_type = text; result<text> parse(char const*&, char const*) const noexcept; };
struct relative_json_pointer_rule_t { using value_type = text; result<text> parse(char const*&, char const*) const noexcept; };

// Boost.URL's uri_reference_rule, then the guard of section 5.7.
struct uri_reference_rule_t {
    using value_type = boost::urls::url_view;
    result<boost::urls::url_view> parse(char const*&, char const*) const noexcept;
};

// RFC 3987, mapped (section 5.8). The resource provides a mapped URI beyond
// the inline capacity; null means std::pmr::get_default_resource().
struct iri_rule_t {
    using value_type = text;
    std::pmr::memory_resource* resource = nullptr;
    result<text> parse(char const*&, char const*) const;
};
struct iri_reference_rule_t {
    using value_type = text;
    std::pmr::memory_resource* resource = nullptr;
    result<text> parse(char const*&, char const*) const;
};

struct uri_template_rule_t { using value_type = text; result<text> parse(char const*&, char const*) const noexcept; };
struct hostname_rule_t     { using value_type = text; result<text> parse(char const*&, char const*) const noexcept; };
struct idn_hostname_rule_t { using value_type = text; result<text> parse(char const*&, char const*) const noexcept; };
struct idna_label_rule_t   { using value_type = text; result<text> parse(char const*&, char const*) const noexcept; };
struct email_rule_t        { using value_type = text; result<text> parse(char const*&, char const*) const noexcept; };
struct idn_email_rule_t    { using value_type = text; result<text> parse(char const*&, char const*) const noexcept; };

inline constexpr date_rule_t date_rule{};
inline constexpr time_rule_t time_rule{};                    // time_rule_t{leap_seconds::refused} for v1
inline constexpr date_time_rule_t date_time_rule{};
inline constexpr duration_rule_t duration_rule{};
inline constexpr uuid_rule_t uuid_rule{};
inline constexpr json_pointer_rule_t json_pointer_rule{};
inline constexpr relative_json_pointer_rule_t relative_json_pointer_rule{};
inline constexpr uri_reference_rule_t uri_reference_rule{};
inline constexpr iri_rule_t iri_rule{};
inline constexpr iri_reference_rule_t iri_reference_rule{};
inline constexpr uri_template_rule_t uri_template_rule{};
inline constexpr hostname_rule_t hostname_rule{};
inline constexpr idn_hostname_rule_t idn_hostname_rule{};
inline constexpr idna_label_rule_t idna_label_rule{};
inline constexpr email_rule_t email_rule{};
inline constexpr idn_email_rule_t idn_email_rule{};

}  // namespace webcpp::formats::grammar
```

Each type is declared in the header of its format (section 3.2). `ipv4`
and `ipv6` have no rule of formats': their rules are Boost.URL's
(`boost::urls::ipv4_address_rule`, `ipv6_address_rule`), which the page
names. `uri` has one, `grammar::uri_rule`, as `uri_reference` does: Boost.URL's
`uri_rule` with the guard of section 5.7, which refuses what RFC 3986 refuses
and Boost.URL 1.92 accepts (a `[` or `]` outside an IP-literal host), so that
`is_uri` and its rule agree (amended 2026-10-09, formats Task 7). `regex` has
none (section 5.12).

**Where a rule stops**, inside a larger grammar: an ASCII rule stops where
its grammar does; `hostname_rule` at the first character outside `[A-Za-z0-9.-]`;
`idn_hostname_rule` at the first ASCII character outside `[A-Za-z0-9.-]`
(any non-ASCII byte belongs to the run, since UTS 46 may map it), and
`idna_label_rule` also at `.`, U+3002, U+FF0E and U+FF61; the IRI rules at
the first byte that no IRI holds (section 5.8); then each validates the run
it took and fails as a whole rather than match a shorter prefix.

### 4.3 An IDN rule composed with Boost.URL's combinators

`idna_label_rule` is a terminal rule of formats' own, UTF-8-aware, that asks
`webcpp::idna` for the verdict:

```cpp
namespace webcpp::formats::grammar {

namespace urlg = boost::urls::grammar;

result<text> idna_label_rule_t::parse(char const*& it, char const* end) const noexcept {
    char const* const first = it;
    // One label: up to '.', U+3002, U+FF0E, U+FF61, or an ASCII byte no label holds.
    char const* const last = detail::scan_idn_run(first, end, detail::idn_run::label);
    if (last == first) {
        return urlg::error::mismatch;   // no label starts here: a variant_rule tries its next rule
    }
    std::array<char, idna::max_domain_length> ascii{};
    if (auto converted = idna::to_ascii(ascii, std::string_view(first, last)); !converted) {
        return converted.error();       // idna's reason: contexto, bidi, not_idna2008, ...
    }
    it = last;
    return text(first, static_cast<std::size_t>(last - first));
}

}  // namespace webcpp::formats::grammar
```

`idn_email_rule` is Boost.URL's combinators over formats' terminals and
Boost.URL's own IP rules, joined by a function rather than a `variant_rule`
(amended 2026-10-09 after the F6 review, to match the code): a `variant_rule`
answers `mismatch` when every alternative fails, which would lose the
`idna::error` that refused the domain (section 6).

```cpp
namespace webcpp::formats::detail {

namespace urlg = boost::urls::grammar;

// "[" IPv4-address-literal "]" or "[IPv6:" IPv6-addr "]" (RFC 5321 4.1.3).
inline constexpr auto address_literal_rule = urlg::tuple_rule(
    urlg::squelch(urlg::delim_rule('[')),
    urlg::variant_rule(
        boost::urls::ipv4_address_rule,
        urlg::tuple_rule(urlg::squelch(ci_literal_rule("IPv6:")), boost::urls::ipv6_address_rule)),
    urlg::squelch(urlg::delim_rule(']')));

// RFC 5321 4.1.2 / RFC 6531 3.3 Mailbox: a local part, "@", then an address literal when the
// domain starts with '[', else the host rule, whose own error (idna's, for an IDN host) is kept;
// a Mailbox longer than max_mailbox_length (254) octets is invalid (section 5.11).
template <class Host>
grammar::result<grammar::text> parse_mailbox(char const*& it, char const* end,
                                             const local_part_rule_t& local,
                                             const Host& host) noexcept;

}  // namespace webcpp::formats::detail

namespace webcpp::formats::grammar {

result<text> idn_email_rule_t::parse(char const*& it, char const* end) const noexcept {
    return detail::parse_mailbox(it, end, detail::idn_local_part_rule, idn_hostname_rule);
}

}  // namespace webcpp::formats::grammar
```

A user composes formats' rules the same way, for instance a host and an
optional port, the host internationalized:

```cpp
namespace urlg = boost::urls::grammar;
namespace fg = webcpp::formats::grammar;

constexpr auto idn_host_port = urlg::tuple_rule(
    fg::idn_hostname_rule,
    urlg::optional_rule(urlg::tuple_rule(urlg::squelch(urlg::delim_rule(':')),
                                         urlg::unsigned_rule<std::uint16_t>{})));

auto parsed = urlg::parse("bücher.example:8080", idn_host_port);
// std::get<0>(*parsed) == "bücher.example", and *std::get<1>(*parsed) == 8080
```

Two Boost.URL facts constrain the compositions (section 13): a
`range_rule` over a rule larger than 128 bytes stores it through Boost.URL's
`recycled_ptr`, a process-wide pool that allocates, so formats passes nested
rules to `range_rule` through `urlg::ref(...)`; and `parse(string_view, r)`
fails with `leftover` when the rule stops early, which is how a format refuses
trailing input (`example.com\n`).

### 4.4 `config.hpp`, `<webcpp/formats.hpp>`

`config.hpp` is idna's form with `WEBCPP_FORMATS_NO_EXCEPTIONS`, including
`<boost/config.hpp>` until `webcpp::config` exists (then one line, as idna's
section 3.4). `<webcpp/formats.hpp>` includes every public header but
`regex.hpp`, and names in its comment the header it leaves out and why
(AGENTS chapter 3).

### 4.5 `detail` (named, not frozen)

`detail::decode_utf8` (P12); `detail::is_ucschar`, `is_iprivate`,
`is_bidi_format` (RFC 3987 2.2, 4.1); `detail::charsets` (`lut_chars` of
each ASCII grammar); `detail::exactly<N>(cs)`, `two_digits_rule`,
`ci_literal_rule`; `detail::refuses_relative_authority_path(url_view)` (the
guard); `detail::map_iri` with `detail::iri_inline_capacity` (1,024);
`detail::scan_idn_run`; `detail::days_in_month(year, month)`;
`detail::idn_local_part_rule`, `local_part_rule`, `address_literal_rule`,
`parse_mailbox`; `detail::template_literals_rule`,
`template_expression_rule`.

## 5. Algorithms and rules

### 5.1 Dates and times (RFC 3339)

- `date_rule`, RFC 3339 5.6 `full-date`: `4DIGIT "-" 2DIGIT "-" 2DIGIT`,
  ASCII digits only; month 01 to 12; day 01 to the month's length, February
  29 only in a Gregorian leap year (divisible by 4, and not by 100 unless by
  400: 0400 yes, 0100 and 2100 no), RFC 3339 5.7.
- `time_rule`, `full-time`: `time-hour ":" time-minute ":" time-second
  [time-secfrac] time-offset`; hour 00 to 23, minute 00 to 59, second 00 to
  60; `time-secfrac` is `"." 1*DIGIT`; `time-offset` is `Z`, or `+` or `-`
  then `hh:mm` with hh 00 to 23 and mm 00 to 59 (`+01` alone is invalid).
  `T` and `Z` in either case (RFC 3339 5.6, NOTE). Second 60 is valid only
  when `leap` is `allowed` and the time, moved to UTC by its offset, is 23:59
  (`15:59:60-08:00` yes, `23:59:60+01:00` no); the date of a leap second is
  not checked against IERS's list (no table, deterministic).
- `date_time_rule`: `full-date ("T" / "t") full-time`, leap seconds allowed.
- `duration_rule`, RFC 3339 appendix A, exactly:
  `"P" (dur-date / dur-time / dur-week)`, `dur-date = (dur-day / dur-month /
  dur-year) [dur-time]`, `dur-time = "T" (dur-hour / dur-minute /
  dur-second)`, `dur-year = 1*DIGIT "Y" [dur-month]`, `dur-month = 1*DIGIT
  "M" [dur-day]`, `dur-hour = 1*DIGIT "H" [dur-minute]`, `dur-minute =
  1*DIGIT "M" [dur-second]`, and `dur-week = 1*DIGIT "W"` alone; no
  fraction, no sign, any number of digits (no overflow: digits are not
  converted).

Built from `urlg::tuple_rule`, `variant_rule`, `optional_rule`,
`delim_rule`, `token_rule(digit_chars)` and `detail::exactly<N>`, with the
range checks in the terminal rules.

### 5.2 UUID (RFC 9562 section 4)

`8HEXDIG "-" 4HEXDIG "-" 4HEXDIG "-" 4HEXDIG "-" 12HEXDIG`, any case; the
version and variant nibbles are not checked (the suite accepts version 15 and
an undefined variant). No braces, no `urn:uuid:`.

### 5.3 JSON Pointer (RFC 6901 section 3)

`*( "/" reference-token )`, `reference-token = *( unescaped / escaped )`,
`escaped = "~" ( "0" / "1" )`, `unescaped` any code point but `/` and `~`
(`%x00-2E / %x30-7D / %x7F-10FFFF`), so the input is decoded as UTF-8 and
must be well-formed. The empty string is valid.

### 5.4 Relative JSON Pointer (draft-handrews-relative-json-pointer-01, section 3; draft-bhutton-relative-json-pointer-00, section 3)

`non-negative-integer [index-manipulation] ( json-pointer / "#" )`, with
`non-negative-integer = "0" / %x31-39 *DIGIT` (no sign, no leading zero) and
`index-manipulation = ( "+" / "-" ) non-negative-integer` (the draft's ABNF,
so `0+0` and `0-0` are valid), and a manipulation also before `"#"`, which
the draft's ABNF omits but its own example `0-1#` uses: the union of what
the draft treats as valid (second amendment of 2026-10-09, after measuring
the draft's text; for the owner to confirm). Index manipulation is
a policy (amendment of 2026-10-09, controller ruling under the owner's
overnight go, after the review of F2): draft 2020-12 and v1 cite
draft-bhutton-relative-json-pointer-00, which has it, and drafts 7 and
2019-09 cite draft-handrews-relative-json-pointer-01, which does not;
`is_relative_json_pointer(s, index_manipulation p =
index_manipulation::allowed)`, `refused` for drafts 7 and 2019-09, as P4 does
for leap seconds. The suite at 5b0ee16 tests neither form.

### 5.5 IPv4 and IPv6

`is_ipv4` is `urlg::parse(s, boost::urls::ipv4_address_rule)`: RFC 3986's
`IPv4address`, which is RFC 2673's dotted-quad with `dec-octet` refusing
leading zeros. `is_ipv6` is `ipv6_address_rule`: RFC 4291 2.2's three forms,
no zone id, no prefix length. Both measured 0 failures on the suite
(JSON_SCHEMA_FOR_WEBCPP section 4.2).

### 5.6 URI (RFC 3986)

`is_uri` is `boost::urls::parse_uri(s)` plus the guards of section 5.7,
RFC 3986 section 3's `URI` (absolute, fragment allowed): Boost.URL accepts a
`[` or `]` in a query, a `#` in a fragment and an RFC 6874 zone id, which
RFC 3986 refuses, and formats refuses them (amended 2026-10-09 after the
owner's ruling that formats is faithful to RFC 3986); 0 failures on the
suite.

### 5.7 URI reference, and the guard

`is_uri_reference` is `parse_uri_reference` plus the guard, RFC 3986 4.1 and
4.2: a relative reference with an authority has `path-abempty`, empty or
starting with `/`. Boost.URL 1.92 accepts `//example.com:abc/p` (host
`example.com`, empty port, path `abc/p`) and `//a@b@example.com/` (path
`@example.com/`); with a scheme it refuses both (measured 2026-10-09 against
Boost 1.92):

```cpp
// detail/uri_guard.hpp: true when Boost.URL accepted what RFC 3986 4.2 refuses.
inline bool refuses_relative_authority_path(boost::urls::url_view u) noexcept {
    return !u.has_scheme() && u.has_authority() && !u.encoded_path().empty() &&
           u.encoded_path().front() != '/';
}
```

`uri_reference_rule_t::parse` runs `boost::urls::uri_reference_rule` and
returns `urlg::error::invalid` when the guard refuses. The upstream issue
(the two inputs, the parse result, RFC 3986 4.2's ABNF) is drafted in
increment F3 for the owner to file; the guard and its changelog line leave in
the change that moves to the first Boost release carrying the fix, and its
test stays.

The same defect reaches a reference with a scheme inside a larger grammar: Boost.URL matches a prefix of `http://a@b@x/` where the authority is malformed. `uri_reference_rule` gives `invalid` there too, through `detail::refuses_authority_before_segment(url_view, stop)`, which sees where Boost.URL stopped (added 2026-10-09 after formats' review, controller ruling).

### 5.8 IRI and IRI reference (RFC 3987)

1. **Scan**: the IRI runs to the end of the input, or (inside a larger
   grammar) to the first byte no IRI holds: ASCII controls, space, `"`, `<`,
   `>`, `\`, `^`, `` ` ``, `{`, `|`, `}`.
2. **Check each code point** (strict UTF-8, P12): an ASCII one is left to
   Boost.URL; a non-ASCII one must be `ucschar` (RFC 3987 2.2), or
   `iprivate` when it lies in the query, which is the text after the first
   `?` that precedes the first `#` (RFC 3986 appendix B: no valid IRI holds a
   `?` before its query or a `#` before its fragment); the bidirectional
   formatting characters U+200E, U+200F and U+202A to U+202E are refused
   anywhere (RFC 3987 4.1, a MUST outside the ABNF).
3. **Map** (RFC 3987 3.1 step 2): each octet of a non-ASCII code point
   becomes `%XX`, uppercase hex; ASCII is copied. An ASCII input skips steps
   2 and 3 and goes to Boost.URL as it is.
4. **Parse** the mapped URI with Boost.URL: `uri_rule` for `iri`,
   `uri_reference_rule` plus the guard of 5.7 for `iri-reference`.
   Placement follows: a `%XX` is valid exactly where RFC 3986 allows
   `pct-encoded`, which is where RFC 3987 allows `ucschar` (`iuserinfo`,
   `ireg-name`, `ipath`, `iquery`, `ifragment`), so a non-ASCII code point in
   a scheme, a port or an IP literal is refused by Boost.URL.
5. **Position**: inside a larger grammar, the length Boost.URL consumed of the
   mapped text is translated back to the input (every non-ASCII code point
   maps to whole triplets, and Boost.URL never stops inside a run of them,
   since a `%XX` is valid wherever the one before it was).

Storage per P5: none for ASCII, the inline buffer up to 1,024 mapped octets,
the resource beyond.

### 5.9 URI Template (RFC 6570 section 2, erratum 6937)

```
URI-Template  = *( literals / expression )
literals      = %x21 / %x23-24 / %x26-3B / %x3D / %x3F-5B / %x5D / %x5F
              / %x61-7A / %x7E / ucschar / iprivate / pct-encoded
expression    = "{" [ operator ] variable-list "}"
operator      = "+" / "#" / "." / "/" / ";" / "?" / "&" / "=" / "," / "!" / "@" / "|"
variable-list = varspec *( "," varspec )
varspec       = varname [ ":" max-length / "*" ]
varname       = varchar *( ["."] varchar )
varchar       = ALPHA / DIGIT / "_" / pct-encoded
max-length    = %x31-39 0*3DIGIT
```

Erratum 6937 (verified 2022-05-06) widens `%x26 / %x28-3B` to `%x26-3B`, the
apostrophe included (the suite's `a'b` is valid). The operators of
`op-reserve` are accepted, as the ABNF accepts them. Built as
`urlg::range_rule` over `urlg::variant_rule(detail::template_literals_rule,
detail::template_expression_rule)`; the literals rule is the UTF-8-aware
terminal that reuses `detail::is_ucschar` and `is_iprivate`; `varname` is
`not_empty_rule(pct_encoded_rule(varchar_chars))` separated by single dots.

### 5.10 Host names

- `hostname_rule`: ASCII only; labels separated by `.`, none empty (no
  leading, trailing or double dot); each 1 to 63 octets of letters, digits
  and `-`, not starting or ending with `-`; the name at most 253 octets
  (RFC 1123 2.1, RFC 1034 3.1). A label starting with `xn--` in any case is
  an A-label: lowercased into a 63-octet stack buffer and checked with
  `idna::check_label` (P9), which decodes it, applies IDNA2008 and the
  per-label Bidi rule, and refuses a non-canonical or all-ASCII result.
  Other labels with `--` in positions 3 and 4 are valid (RFC 1123).
- `idn_hostname_rule`: `idna::to_ascii` of the run, with idna's default
  options (UTS 46 mapping, every check, `verify_dns_length`, IDNA2008), into
  a `std::array<char, idna::max_domain_length>`. The suite's mapping cases
  (fullwidth digits, U+200B ignored, non-NFC input, U+3002, U+FF0E and U+FF61
  as dots) and its strict cases (DISALLOWED exceptions, CONTEXTO, CONTEXTJ,
  the Bidi rule across labels, `xn--7a`) are all idna's.
- `idna_label_rule`: one label of the above (section 4.3).

### 5.11 E-mail addresses

- `email_rule`, RFC 5321 4.1.2 `Mailbox = Local-part "@" ( Domain /
  address-literal )`: `Local-part = Dot-string / Quoted-string`, at most 64
  octets (4.5.3.1.1); `Dot-string = Atom *("." Atom)`, `Atom = 1*atext`
  (RFC 5322 3.2.3's `atext`); `Quoted-string = DQUOTE *QcontentSMTP DQUOTE`,
  `qtextSMTP = %d32-33 / %d35-91 / %d93-126`, `quoted-pairSMTP = %d92
  %d32-126`; the domain is `hostname_rule` (P10); `address-literal` is
  `"[" IPv4-address-literal "]"` or `"[IPv6:" IPv6-addr "]"` (the tag in any
  case), through Boost.URL's IP rules.
- `idn_email_rule`, RFC 6531 3.3: `atext =/ UTF8-non-ascii`,
  `qtextSMTP =/ UTF8-non-ascii` (RFC 6532 3.1: any well-formed non-ASCII
  sequence, C1 controls and noncharacters included); `quoted-pairSMTP`
  stays ASCII; the local part is at most 64 octets; the domain is
  `idn_hostname_rule` or an address literal (section 4.3's composition). The
  local part is not normalized (a non-NFC one is valid, as the suite says).
- A Mailbox longer than 254 octets is refused: RFC 5321 4.5.3.1.3 bounds a
  path at 256 octets with its angle brackets (amended 2026-10-09 after the
  F5 review, controller ruling; for the owner to confirm).
- For `idn_email_rule`, the 254 octets of the Mailbox and the 64 of its
  local part are octets of the Mailbox as written, in UTF-8, not of its
  ASCII form: RFC 6531 sets no limit of its own, RFC 5321's are octets, and
  an SMTPUTF8 envelope carries the address as written (amended 2026-10-09
  after the F6 review, controller ruling; for the owner to confirm with the
  254). The domain's own DNS lengths (63 per label, 253 per name) stay
  those of its ASCII form, as `idn_hostname_rule` checks them.

### 5.12 Regular expressions (`<webcpp/formats/regex.hpp>`)

`is_regex(pattern)`: 1. webcpp::regex refuses the escapes `\A`, `\Z` and
`\z` (regex P6), ill-formed UTF-8, and a pattern past `regex::max_nesting`
(`too_deep`) or `regex::max_alternatives` (`too_complex`), before any engine
sees it. 2. Compile with `regex::regex::compile(pattern, {.syntax =
ECMAScript, .memory = &resource})` and answer `has_value()`; SRELL's
`SRELL_NO_UBMOD`, a usage requirement of the srell target, refuses a leading
`(?i)`. The verdict is the configured backend's: ECMA-262 `u` mode on SRELL
and Boost.Regex, C++'s ECMAScript on std::regex (where `format: regex` is
not JSON Schema's), which the page says, linking regex's matrix. A pattern
past a bound is refused without being judged, `false`, as for a pattern
ECMA-262 refuses; the page says so. No rule: webcpp::regex compiles a whole
string and reports no position, so a `regex_rule` could only consume all of
its input, which composes with nothing. The page says so.

## 6. Errors

- The functions return `bool`; no error category of formats' own (P3).
- A rule's error: `urlg::error::mismatch` (did not start here), or
  `urlg::error::invalid` (started, then broke a constraint), or Boost.URL's
  own errors from its rules (`leftover` from `parse` on trailing input), or,
  from an IDN rule, the `idna::error` that refused the run. A caller
  composing rules may print the reason; `is_*` discards it.
- `noexcept` holds for every function and rule but the IRI ones (a resource
  may throw `std::bad_alloc`) and `is_regex` (its resource may throw
  `std::bad_alloc`). With the default resource in a build without
  exceptions, a failed allocation aborts, as `tools/throw_exception.cpp`
  does; `is_regex` allocates through its resource, or the default resource,
  whose failure propagates likewise.
- No API throws, so nothing sits behind `WEBCPP_FORMATS_NO_EXCEPTIONS`,
  whose Doc Comment says formats raises no exception of its own.

## 7. Allocation

| Function or rule | Allocates |
| --- | --- |
| every `is_*` but those below, every rule but the IRI ones | never (Boost.URL's views and formats' rules hold no storage; idna's span forms; `range_rule` only over rules within its 128-byte inline buffer) |
| `is_iri`, `is_iri_reference`, `iri_rule`, `iri_reference_rule` | never for ASCII input or a mapped URI of up to 1,024 octets; beyond, one buffer of the mapped length from the given resource, else `std::pmr::get_default_resource()` (P5) |
| `is_regex` | webcpp::regex's compilation, only through the resource given, else the default resource, all of it given back before it returns, where `regex::honours<>::memory` holds (P6) |

Proof: `allocation_test` replaces `operator new`/`delete` for itself and
counts zero across every validator but the two above on long and hostile
inputs (a 100,000-octet hostname, an IRI of 341 non-ASCII octets), counts
that an IRI past the inline capacity allocates only through a counting
`memory_resource` passed to it, and that `is_regex` allocates only through
its resource.
Stack: the IRI buffer (1 KiB) and idna's per-label storage (about 1.3 KiB),
measured in F8 on wasip2 at `-O0` against the 64 KiB default stack.

## 8. The build

### 8.1 Targets and Jamfiles

- `build.jam` as in section 3.3; header-only, but a program links
  `/webcpp//boost_url` (S4), a static library built once per variant, as
  `/webcpp//boost_json` is.
- `test/Jamfile`: `webcpp.targets native wasip2 wasip3 emscripten ;`,
  `webcpp.headers-alone formats : ../include ;`, `config` compiled twice, one
  `webcpp.run` per unit test, and the suite runner (section 9.1).
- `example/Jamfile`, `doc/Jamfile` as AGENTS chapters 8 and 9 say.

### 8.2 Boost.URL in C++20

Measured 2026-10-09 (Boost 1.92, Apple clang, `-std=c++20`): `parse_uri`,
`parse_uri_reference`, `url_view`'s accessors, the combinators,
`ipv4_address_rule` and `ipv6_address_rule` are defined in the headers
(`BOOST_URL_CXX20_CONSTEXPR`); a program using all of them, with and without
exceptions, fails to link on exactly two symbols,
`ipv4_address::ipv4_address(bytes_type const&)` and
`ipv4_address::to_bytes() const`, which live in Boost.URL's compiled
sources. Linked with the compiled library it runs, the guard included. So
formats needs the compiled Boost.URL, though little of it.

### 8.3 Superproject prerequisites (shared with json_schema)

| | What | Needed by | Owner |
| --- | --- | --- | --- |
| **S2** | `third_party/` in the superproject: its `Jamfile` (`/webcpp/third_party`), and the submodule `third_party/libs/json-schema-test-suite` (json-schema-org/JSON-Schema-Test-Suite, pinned; this specification's counts are at `5b0ee16`, 2026-09-21); a path constant the tests read | F1's suite slice | json_schema's decision of section 10; the path json_schema's spec gives wins |
| **S3** | the mirror `webcpporg/srell` (the owner creates it): one commit importing SRELL 2026.07's zip exactly (URL and SHA-256 `d384feae20c66b4d65bc240ee2a851aed26ca24dc4d73503eee3e81e238ec20e` in the message), one commit with the patch of section 8.4; the submodule `third_party/libs/srell`; the target `/webcpp/third_party//srell`, an alias with usage requirements `<include>libs/srell <define>SRELL_NO_UBMOD` | regex (formats no longer depends on it) | decision 11 |
| **S4** | `/webcpp//boost_url`: Boost.URL's compiled sources, of the configured Boost's version, built static once per variant on every target, as `boost_json` is. The installed Boost has no sources (Homebrew's ships `libboost_url` and no `libs/url/src`; the CI installs headers only), so the Jamroot needs a Boost source tree: proposed, `-sBOOST_SOURCE=<dir>`, else `.local/boost-source`, the same tarball the Boost action already downloads and checks, its `libs/url/src` kept; the Jamroot refuses a tree whose `boost/version.hpp` differs from the configured Boost's. This is AGENTS chapter 13's "Compiled Boost libraries", started by formats | F3 | the owner decides the mechanism |
| **S5** | test data on WASI: a test reaches the suite's files under wasmtime (`--dir`) through the build's run of WASI tests, json_schema's section 10 mechanism | F8 | json_schema's |

Each has its tests in `tools/test/` red first, as AGENTS chapter 10 asks of a
change of the build. S2 to S5 are independent of each other.

### 8.4 The SRELL patch

In the mirror, a commit of its own, also offered upstream (an owner's
action):

- when the compiler has no exceptions (`__cpp_exceptions` undefined) and
  `SRELL_NO_THROW` is undefined, define `SRELL_NO_THROW`, so SRELL's
  `regex_error` sites compile out;
- each of the four `throw std::bad_alloc();` (`srell.hpp` lines 1055, 1425,
  1550, 1561 in 2026.07) becomes the throw when exceptions are on and
  `std::abort()` when they are off, the policy of webcpp's
  `throw_exception.cpp`.

`SRELL_NO_UBMOD` is a usage requirement of the srell target, the
superproject's; formats no longer depends on the srell target, which only
`/webcpp/regex//regex` names.

## 9. Tests

### 9.1 The JSON Schema suite, run against the functions

`test/suite_test.cpp`, one program, reads every file of
`tests/<draft>/optional/format/` for `draft4`, `draft6`, `draft7`,
`draft2019-09`, `draft2020-12`, and `tests/v1/format/`, with Boost.JSON,
the directory given as xstate's fixtures are (`path-constant`, a define,
the files as `<dependency>`). For each case it takes the schema's `format`
and runs each string test against the function of section 1, comparing with
`valid`:

- `time` in v1 runs `is_time(s, leap_seconds::refused)`, and
  `relative-json-pointer` in drafts 7 and 2019-09 runs
  `is_relative_json_pointer(s, index_manipulation::refused)`; everything
  else is the same call in every draft;
- `regex.json` and `ecmascript-regex.json` (format `regex`) run `is_regex`;
- a non-string instance (formats apply to strings only) is checked to expect
  `valid: true`, and counted apart, since it tests json_schema's rule rather
  than a function here;
- `unknown.json` (format `unknown`, drafts 4 to 2020-12) is skipped by name:
  an unknown format is json_schema's question; the runner counts it so that
  no file passes unnoticed, and fails on any other file it cannot map.

The files and their string tests (strings / all tests) at `5b0ee16`, every
draft's data identical unless noted:

| File | draft4 | draft6 | draft7 | 2019-09 | 2020-12 | v1 |
| --- | --- | --- | --- | --- | --- | --- |
| `date-time.json` | 37/43 | 37/43 | 37/43 | 37/43 | 37/43 | 37/43 |
| `date.json` | | | 75/81 | 75/81 | 75/81 | 75/81 |
| `time.json` | | | 49/55 | 49/55 | 49/55 | 49/55, six leap-second verdicts inverted |
| `duration.json` | | | | 46/52 | 46/52 | 46/52 |
| `email.json` | 65/71 | 65/71 | 65/71 | 65/71 | 65/71 | 65/71 |
| `idn-email.json` | | | 18/24 | 18/24 | 18/24 | 19/25 |
| `hostname.json` | 25/31 | 25/31 | 58/64 | 58/64 | 58/64 | 58/64 |
| `idn-hostname.json` | | | 92/98 | 92/98 | 92/98 | 92/98 |
| `ipv4.json` | 35/41 | 35/41 | 35/41 | 35/41 | 35/41 | 35/41 |
| `ipv6.json` | 45/51 | 45/51 | 45/51 | 45/51 | 45/51 | 45/51 |
| `uri.json` | 41/47 | 41/47 | 41/47 | 41/47 | 41/47 | 41/47 |
| `uri-reference.json` | | 25/31 | 25/31 | 25/31 | 25/31 | 25/31 |
| `iri.json` | | | 20/26 | 20/26 | 20/26 | 20/26 |
| `iri-reference.json` | | | 15/21 | 15/21 | 15/21 | 15/21 |
| `uri-template.json` | | 41/47 | 41/47 | 41/47 | 41/47 | 41/47 |
| `json-pointer.json` | | 34/40 | 34/40 | 34/40 | 34/40 | 34/40 |
| `relative-json-pointer.json` | | | 20/26 | 20/26 | 20/26 | 20/26 |
| `uuid.json` | | | | 23/29 | 23/29 | 23/29 |
| `regex.json` | | | 2/8 | 2/8 | 2/8 | 2/8 |
| `ecmascript-regex.json` | | | 12/12 | 12/12 | 12/12 | 12/12 |
| `unknown.json` (skipped) | 1/7 | 1/7 | 1/7 | 1/7 | 1/7 | |
| **string tests run** | **248** | **348** | **684** | **753** | **753** | **754** |

3,540 string tests in all (the per-draft totals of every test, 291, 409,
793, 874, 874 and 868, match JSON_SCHEMA_FOR_WEBCPP section 3.3). The draft
4 and draft 6 `hostname.json` holds 25 strings of its own (among them
`ab--cd.example`, valid), none contradicting draft 7's 58.

Lanes: native and emscripten (`-sNODERAWFS=1` reads the host's files) from
F1; wasip2 and wasip3 once S5 lands (F8). Until then the runner is declared
`: native emscripten`, with a comment naming S5; the unit tests run on all
four targets from the start.

### 9.2 Known failures

`test/known_failures/<dialect>.json`, json_schema's shape (json_schema spec
16.1): `{file, case, test, reason}`, `file` relative to the draft's
directory (`optional/format/uuid.json`), `case` and `test` by description. The runner fails on a
failure the list does not name and on a listed test that passes. Target
100%: every list is expected empty at F8. The only entry allowed is a residue
tied to a Unicode version (idna's tables are 17.0.0; a suite test written
against another version's properties), its reason naming both versions.

### 9.3 Unit tests and RFC examples

lightweight_test, `webcpp.run`, on native, wasip2, wasip3 and emscripten:

| Test | Holds |
| --- | --- |
| `config` (twice) | the macro |
| `date_time_test` | RFC 3339 5.8's five examples; every month's last day and the next; February 29 for 0400, 0100, 2000, 2100, 2020, 2021; leap seconds at each offset, both policies; `T`/`t`, `Z`/`z`; secfrac of one and of 30 digits; RFC 3339 appendix A's duration forms, and each refused shape |
| `uuid_test` | the canonical forms, each wrong length and dash position, braces, `urn:uuid:` |
| `json_pointer_test` | RFC 6901 section 5's twelve pointers; every `~` escape; ill-formed UTF-8 refused; relative pointers from the draft's section 5.1 examples |
| `ip_test` | RFC 4291 2.2's examples; leading zeros; zone ids and prefixes refused |
| `uri_test` | RFC 3986 1.1.2's eight examples; the guard's two inputs, refused, and the same with a scheme; `//example.com/p` valid; the zone id verdict pinned (P11) |
| `iri_test` | the prototype's 27 IRI probes (URI_TEMPLATE_AND_IRI_PROBE, `suite.py` `EXTRA`): IPv4-like reg-names, IPvFuture, zone ids, truncated, overlong and surrogate UTF-8, `iprivate` outside the query, U+0080, U+FFFD, the bidi formatting characters; non-ASCII in a scheme, a port, an IP literal; the inline capacity's edge (341 and 342 non-ASCII octets) |
| `uri_template_test` | RFC 6570 section 1.2's level 1 to 4 templates (valid); the prototype's 22 template probes (`{`, `}`, `{v*:3}`, `{a.}`, `{v:}`, and the forbidden literals: less-than, double quote, caret, vertical bar, backslash, backtick) |
| `hostname_test` | label and name lengths at 63/64 and 253/254; A-labels in both cases; R-LDH labels in both formats (P9); the 15 IDN strings of IDNA_SPIKE section 2.3 and their A-labels |
| `email_test` | each production of 5.11 and its refusal; the local part at 64 and 65 octets; `[IPv6:...]` in both cases; a General-address-literal refused |
| `regex_test` | `\A`, `\Z`, `\z` refused, `\\A` accepted; `(?i)` refused; ill-formed UTF-8 refused, NUL a literal; 10,000 nested groups `too_deep`, a value; 10,000 sequential groups compile; `max_alternatives` alternatives compile and one more is refused; every allocation through the resource given (an integration test of formats over webcpp::regex) |
| `grammar_test` | every public rule inside `tuple_rule`, `variant_rule`, `optional_rule` and `range_rule`; where each stops (section 4.2); section 4.3's host-and-port example; the error an IDN rule returns is idna's |
| `allocation_test` | section 7 |
| `headers-alone formats` | every public header, `regex.hpp` included |

A failure found by the suite that points to idna becomes a test in idna,
red first (WEBCPP_IDNA_SPEC section 9).

## 10. Documentation

- Doc Comments on every public symbol, strict MrDocs reference, as idna's
  section 10 says; each rule type's comment names its RFC section and where
  it stops.
- `doc/formats.adoc`: an introduction (the table of section 1, a first
  example); dates and times; identifiers (UUID, JSON Pointers); addresses (IP,
  host names, e-mail, with the idna link
  `link:{webcpp-libs}/idna/{webcpp-page}#...`); URIs and IRIs (Boost.URL, the
  guard and its upstream issue, the mapping and its storage); URI templates;
  regular expressions (webcpp::regex, the escapes refused, the bounds, the
  allocation, linking `link:{webcpp-libs}/regex/{webcpp-page}#regex-matrix[...]`
  once regex's R9 lands); composing
  rules (section 4.3's examples, the two Boost.URL constraints); errors;
  allocation; the JSON Schema suite (a conformance table per draft, its counts
  from `doc/counts.py` reading the suite and the known-failure lists, never
  typed); guarantees; testing; the reference; an appendix "Known deviations"
  (where formats refuses what Boost.URL accepts: brackets in a query, a `#` in a fragment, P11's zone ids; where Boost.URL refuses what RFC 3986 accepts: an uppercase `V` in IPvFuture; v1's RFC 9557 and published leap seconds; SRELL's
  extensions, refused by webcpp::regex; the bounds `max_nesting` and
  `max_alternatives`).
- Guarantees (`formats-invariant-<n>`): (1) `is_x(s)` and
  `parse(s, x_rule)` agree for every format; (2) no function allocates but
  those of section 7's table; (3) nothing throws but an allocation; (4) every
  function refuses ill-formed UTF-8; (5) a rule that fails leaves its
  iterator where it was.
- Examples, compiled and run on every target with their `.expected`:
  `formats_quick_start.cpp` (a few `is_*`), `formats_grammar.cpp` (section
  4.3's composition), `formats_iri.cpp` (an IRI with a resource),
  `formats_regex.cpp`.

## 11. The changelog

As idna's section 11. F0 writes, under "New Libraries":

- formats: validators and Boost.URL grammar rules for the string formats of
  JSON Schema (dates and times, e-mail addresses, host names, IP addresses,
  URIs, IRIs, URI templates, JSON Pointers, UUIDs, regular expressions), for
  C++20, from R.

Later increments refine it until the first release; after it, every
user-visible change adds a bullet, the guard's removal among them.

## 12. Increments

Each ends with the named tests passing from scratch on its lanes, the lint
clean, the page building, and a commit. **∥** marks what can run in
parallel. The idna increments (I0 to I6) are in WEBCPP_IDNA_SPEC section 12.

**Superproject prerequisites (section 8.3), all ∥ each other and ∥ F0 to F2:**
S2 (suite submodule), S3 (SRELL mirror, after the owner creates the
repository), S4 (`boost_url`, after the owner's choice of mechanism), S5 (WASI
data, json_schema's).

**formats.**

- **F0. Skeleton.** The tree with an empty convenience header, `config.hpp`,
  `build.jam` (idna only; Boost.URL joins with F3, webcpp::regex with F7), `meta/`,
  CI file, README, AGENTS.md, the page's introduction and reference.
  Passing: `headers-alone`, `config` twice, on the four targets. ∥ I0.
- **F1. Dates and times.** `date_time.hpp`, `detail/rules.hpp`, the suite
  runner. Needs S2 for its suite slice. Passing: `date_time_test` on the four
  targets; the suite's `date-time`, `date`, `time` (both policies) and
  `duration` files at 100% on native and emscripten. ∥ F2, F3, F7, idna.
- **F2. UUID and JSON Pointers.** `uuid.hpp`, `json_pointer.hpp`,
  `detail/utf8.hpp`. Passing: `uuid_test`, `json_pointer_test`; the suite's
  `uuid`, `json-pointer`, `relative-json-pointer`. ∥ F1, F3, F7, idna.
- **F3. IP addresses and URIs.** `ip.hpp`, `uri.hpp` (`uri`,
  `uri-reference`, the guard), the upstream issue drafted. Needs S4.
  Passing: `ip_test`, `uri_test`; the suite's `ipv4`, `ipv6`, `uri`,
  `uri-reference`. ∥ F1, F2, F7, idna.
- **F4. IRIs and URI templates.** The IRI half of `uri.hpp`,
  `uri_template.hpp`, `detail/iri_mapping.hpp`. Needs F2 (UTF-8), F3.
  Passing: `iri_test`, `uri_template_test`, the IRI part of
  `allocation_test`; the suite's `iri`, `iri-reference`, `uri-template`.
  ∥ F7, idna.
- **F5. Host names and e-mail.** `hostname.hpp` (`hostname_rule`),
  `email.hpp` (`email_rule`). Needs F3 (IP rules) and idna's I4
  (`check_label`). Passing: `hostname_test` and `email_test` for the ASCII
  formats; the suite's `hostname` and `email` in every draft. ∥ F4, F7, I5.
- **F6. IDN host names and IDN e-mail.** `idn_hostname_rule`,
  `idna_label_rule`, `idn_email_rule`. Needs F5 and idna's I5
  (`to_ascii`). Passing: the IDN halves of `hostname_test` and `email_test`,
  `grammar_test`; the suite's `idn-hostname` and `idn-email` at 100%.
  ∥ F7, I6.
- **F7. Regular expressions.** `regex.hpp` on webcpp::regex,
  `/webcpp/regex//regex` in `build.jam`. Starts after regex's R1 is merged;
  no longer needs S3. Passing: `regex_test`; the suite's `regex` and
  `ecmascript-regex`. The resource overload's "nothing outside the resource"
  is proven by `allocation_test` once regex's R4 lands, and holds through
  the SRELL backend's resource before. ∥ F1 to F6.
- **F8. Every target, the documentation, readiness.** Needs S5 and F1 to F7.
  The runner declared on wasip2 and wasip3; the page complete with its
  conformance table, examples and guarantees; the known-failure lists final
  (expected empty); the whole `allocation_test` and the stack measurement.
  Passing: `b2 -a libs/formats/test libs/formats/example` on the four
  targets, `b2 -a libs/formats/doc`, the lint, idna's lanes unchanged.

## 13. Risks

1. **Boost.URL is a compiled library** and webcpp's CI installs Boost's
   headers alone (AGENTS chapters 2 and 13). S4 needs the owner's choice of
   where the sources come from; until it lands, F3, F4, F5 and F6 cannot
   link on any target. Measured: formats' use reaches two compiled symbols
   (section 8.2), and the 19 sources build for wasip2 and wasip3 unchanged
   (ENGINE_SPIKE section 4). Size: a stripped wasip2 program using `is_uri`
   is measured in F3 and recorded on the page.
2. **Shared superproject work.** S2, S3 and S5 are json_schema's too;
   two tracks building them twice, or differently, is the risk. Each is built
   once, on its own branch, by the first track to need it, and the other
   rebases; a name json_schema's spec fixes (the suite's path) wins.
3. **Boost.URL's `recycled_ptr`.** A `range_rule` over a rule larger than
   128 bytes keeps it in a process-wide pool that allocates (`grammar/impl/
   range_rule.hpp`), which is static mutable state and an allocation outside
   any resource. formats passes nested rules through `urlg::ref`, and the
   allocation test fails if the pool is ever reached.
4. **The regex engine is webcpp::regex's configuration**; with SRELL, the
   mirror's commits (regex spec 9.4) carry its fixes until upstream releases
   them.
5. **Zone identifiers** (P11): Boost.URL accepts RFC 6874's form, which
   RFC 9844 withdrew; formats refuses it, faithful to RFC 3986.
6. **The guard's lifetime.** It leaves only with a Boost release that fixes
   the bug, a change of its own; `uri_test` keeps the two inputs so a Boost
   that regresses is caught.
7. **The IRI rule inside a larger grammar** translates Boost.URL's position
   back through the mapping (section 5.8 step 5); `grammar_test` pins it with
   non-ASCII text right before and after the IRI's end.
8. **v1 is unpublished and drifting.** Its suite already inverts the `time`
   leap seconds (handled, P4); its text asks `date-time` to be RFC 9557's
   `date-time-ext` and to check published leap seconds, which no test covers
   yet. When v1 is published, formats adds them as a contract extension (a
   new function or policy, never a changed default), announced to
   json_schema.
9. **The suite moves.** It is pinned by the submodule; a bump is a change
   with its evidence, and a new failing test is either fixed or listed with
   its reason in the same change.
10. **The suite on WASI waits for S5.** Until then the runner proves native
    and emscripten only; the unit tests hold the formats on wasip2 and wasip3
    meanwhile, and F8 is not done without the four.
11. **hostname and idn-hostname differ on R-LDH labels** (P9), by RFC 1123
    and RFC 5890; the page shows both verdicts side by side so the difference
    reads as a decision, not a bug.

**Owner decision (2026-10-09) on S4:** approved and generalised - every compiled Boost library a webcpp library needs is built from Boost's source tarball for each target, as Boost.JSON is (`~/.buzz/PLANS/WEBCPP_DESIGN.md`, owner decisions).

**Owner decision (2026-10-09) on SRELL allocation:** the `webcpporg/srell` mirror carries an allocator patch as its third commit (after the import and the no-exceptions patch), offered upstream; `is_regex` gains a `std::pmr::memory_resource&` overload and allocates nothing outside it (`~/.buzz/RESEARCH/SRELL_ALLOCATION_SPIKE.md`).

Approved by the owner on 2026-10-09.
