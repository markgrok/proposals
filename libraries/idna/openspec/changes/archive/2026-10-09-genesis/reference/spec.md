---
title: "webcpp::idna: specification"
tags: [webcpp, idna, unicode, punycode, spec]
status: active
created: 2026-10-09
---

# webcpp::idna: specification

`webcpp::idna` is a library of webcpp's own (not a port): header-only C++20
internationalized domain names. It implements Punycode (RFC 3492), the
processing of UTS 46 (Unicode IDNA Compatibility Processing, revision 35 for
Unicode 17.0.0: mapping, NFC, ToASCII, ToUnicode), and the validity rules of
IDNA2008 (RFC 5890 to 5893: PVALID, CONTEXTJ, CONTEXTO, NFC, the Bidi rule
across labels), over Unicode tables generated from Unicode 17.0.0 as
`constexpr` data. Its first consumer is `webcpp::formats` (the JSON Schema
formats `hostname`, `idn-hostname`, `email`, `idn-email`), through which
`webcpp::json_schema` and later `webcpp::zod` reach it.

The public interface in section 4 is a contract: `webcpp::formats` is
specified in parallel (`WEBCPP_FORMATS_SPEC.md`) and codes against it before
this library is finished. Where this specification and the superproject's
`AGENTS.md` differ, `AGENTS.md` wins; where it and the owner's decisions
differ, the owner's decisions win.

Sources: the approved design `JSON_SCHEMA_DESIGN.md` section 8 (formats);
the measurements `~/.buzz/RESEARCH/IDNA_SPIKE.md`; the owner's decisions of
2026-10-09 restated in section 2.1; UTS 46 revision 35
(https://www.unicode.org/reports/tr46/tr46-35.html, Unicode 17.0.0,
2025-09-04); RFC 3492, 5890, 5891, 5892, 5893.

## 1. Purpose

- **Punycode**, encode and decode, per RFC 3492, with errors as values.
- **UTS 46 processing**: `to_ascii` and `to_unicode`, with every flag of UTS
  46 section 4 as an option, nontransitional by default.
- **IDNA2008 validity**: `is_valid_label` and `is_valid_domain` (and the
  `check_*` forms that say why), which apply the derived property (PVALID;
  CONTEXTJ and CONTEXTO by their rules; DISALLOWED otherwise), NFC, the
  hyphen and leading-mark rules, and the Bidi rule of RFC 5893 across the
  labels of a domain.
- **A strict profile by default**: UTS 46's mapping, followed by IDNA2008's
  stricter validity (`options::idna2008`), which is what JSON Schema's
  `idn-hostname` asks for and what no measured UTS 46 library passes
  (IDNA_SPIKE section 6: ada 16.3%, upa 15.2%, ICU 4.3% failed).
- **No heap on the validation path**: every validator and every conversion
  into a caller's buffer runs in fixed storage; the overloads that return a
  string take a `std::pmr::memory_resource`.

Not in scope: IDNA2003, registry-specific rules (RFC 5891 section 4.3 leaves
them to registries), confusable detection (UTS 39), and WHATWG's URL host
parser (a later URL library may build on `to_ascii` with its own options).

## 2. Decisions

### 2.1 The owner's (binding)

From the owner's section-by-section approval of 2026-10-09
(`JSON_SCHEMA_DESIGN.md` section 8) and webcpp's rules:

1. **A library of its own**, `webcpp::idna`, repository `webcpporg/idna`
   (created with the owner's go), consumed by `webcpp::formats`.
2. **IDNA2008 / UTS 46.** Punycode `encode`/`decode` per RFC 3492 with errors
   as values; `to_ascii`/`to_unicode`; `is_valid_label`/`is_valid_domain`
   (PVALID, CONTEXTJ/CONTEXTO, NFC, the Bidi rule across labels).
3. **`detail::`** holds UTF-8 decoding and the tables. The tables are
   generated from Unicode 17.0 as `constexpr` data with no heap (about 126 KB
   raw, IDNA_SPIKE section 5), by a Python generator kept in the library's
   own repository, `libs/idna/tools/`. It runs only when Unicode is updated,
   and a check proves the committed tables match what it writes.
4. **Dependencies:** `webcpp::config` only. Until the config library exists
   (`WEBCPP_RELEASE_DESIGN.md` decision 1), `config.hpp` includes
   `<boost/config.hpp>`; the migration is one line (section 3.4).
5. **ada-url is a reference for algorithms only** (and for table packing);
   no code is copied.
6. **Exceptions:** `WEBCPP_IDNA_NO_EXCEPTIONS` in `config.hpp`, as AGENTS
   chapter 6 says; every throw goes through `boost::throw_exception`, and a
   throwing API sits behind the macro. idna is errors-as-values and raises
   nothing of its own (section 6).
7. **Allocation:** pmr allocators wherever anything allocates; ideally
   nothing does (WEBCPP_DESIGN, owner decision 2026-10-08, allocation; pmr as
   for trystero, pratt and json_schema).
8. **Targets:** native, wasip2, wasip3, emscripten.
9. **Tests:** unit tests and the RFC and Unicode conformance vectors; the
   official JSON-Schema-Test-Suite's `optional/format/` cases run in
   `webcpp::formats` against the functions built on this library. Target
   100%; a residue tied to a Unicode version is a tracked known failure; CI
   fails on a new failure and on an unexpected pass.
10. **Documentation:** a page with complete Doc Comments and a strict MrDocs
    reference.
11. **Changelog:** a line in "Unreleased" with every change.
12. **Licence and text:** BSL-1.0, "Copyright (c) 2026 WebCpp.org"; the
    banned word of AGENTS chapter 11 nowhere; English; no em dash.
13. **Size:** about 1,000 to 1,200 lines of library code plus the generator
    (600 to 800 lines).

### 2.2 Proposed by this specification (for the owner's review)

Each is a proposal until the owner accepts it; the reason follows each.

- **P1. Two forms of every conversion.** A form that writes into a caller's
  `std::span` and never allocates, and a form that returns a `std::pmr`
  string from a memory resource. The span form is the primitive; the pmr
  form is a convenience over it. Reason: decision 7's "ideally nothing
  allocates" holds for every DNS-valid name, whose sizes are bounded.
- **P2. Output first** in the span forms, `to_ascii(output, domain)`, as
  Boost.URL's `encode(dest, end, s, ...)` writes, so that no call is
  ambiguous between the two forms.
- **P3. Fixed working storage per label**, `detail::max_label_code_points`
  (256 code points, inline). Every label that can pass `verify_dns_length`
  fits: an A-label holds at most 63 code points, and a label before NFC holds
  at most 4 times its NFC length (the longest full canonical decomposition in
  Unicode 17.0.0 is 4 code points, U+1FAF, measured 2026-10-09). A larger
  label occurs only with `verify_dns_length` off: the pmr forms then take its
  storage from their resource, and the span forms and validators return
  `error::storage_exhausted`.
- **P4. Strict by default.** `options{}` is UTS 46 with every check on,
  nontransitional, `verify_dns_length` on, plus `idna2008 = true`. A caller
  who wants UTS 46 alone sets `idna2008 = false`.
- **P5. The validity of IDNA2008 is derived from the UTS 46 table**: a code
  point is PVALID, CONTEXTJ or CONTEXTO exactly when its UTS 46 status is
  `valid` or `deviation` and it carries no `NV8`/`XV8` mark. Measured
  2026-10-09 against IANA's `idna-tables-12.0.0`: for every code point IANA
  assigns a property, the two agree, except U+002E FULL STOP (DISALLOWED in
  IANA, `valid` in UTS 46 as the label separator, which never reaches a
  label). The generator re-runs this comparison on every generation
  (section 8.3), so no IANA table is shipped.
- **P6. The test vectors are generated too.** The generator writes, besides
  the tables, the vectors of `IdnaTestV2.txt` and the NFC column of
  `NormalizationTest.txt` as C++ data under `test/generated/`, so that the
  conformance tests run on every target with no file system and no network.
- **P7. One error enumerator per rule family** (section 6), not per UTS 46
  status code: IdnaTestV2 itself says an implementation "need only record that
  there is an error".
- **P8. The Unicode data notice** (section 13, risk 1): the generated tables
  derive from Unicode's data files, whose licence (Unicode License v3) asks
  for its notice "with all copies" or "in associated Documentation". Proposed:
  the page's appendix carries the notice, the generated header names it in
  one comment line, and no `LICENSE-UNICODE.txt` is added. This touches
  AGENTS chapter 2's "every file of webcpp is BSL-1.0 only" and needs the
  owner's decision before increment I2 commits generated data.

## 3. Layout and files

### 3.1 Where the work happens

- A worktree of the superproject, `~/dev/webcpp-formats`, on branch
  `lib/formats`, created from `main` and rebased whenever `main` moves; the
  superproject's own checkout `~/dev/webcpp` is not touched. It holds
  `libs/idna` and `libs/formats`, each a git repository of its own
  (`git init -b main`), untracked by the superproject until the owner creates
  `webcpporg/idna` and `webcpporg/formats` and adds the submodules
  (AGENTS chapter 2, checklist items 1 and 2).
- Commits as `R <github.overboard312@passmail.net>`, no trailer, explicit paths
  (AGENTS chapter 11). Nothing is pushed.
- idna needs one superproject change of its own: the check of generated files
  (S1, section 12), shared with json_schema's catalogue check.

### 3.2 The tree

```
libs/idna/
  build.jam                              project /webcpp/idna, alias idna
  meta/libraries.json                    "port-of": null
  meta/include-boundaries.json           section 3.3
  include/webcpp/idna.hpp                every public header
  include/webcpp/idna/config.hpp         WEBCPP_IDNA_NO_EXCEPTIONS
  include/webcpp/idna/errors.hpp         error, its category, result
  include/webcpp/idna/options.hpp        options, the limits, the Unicode version
  include/webcpp/idna/punycode.hpp       punycode::encode, punycode::decode
  include/webcpp/idna/validity.hpp       check_label, check_domain, is_valid_*
  include/webcpp/idna/uts46.hpp          to_ascii, to_unicode
  include/webcpp/idna/detail/utf8.hpp        strict UTF-8 decoding and encoding (RFC 3629)
  include/webcpp/idna/detail/tables.hpp      GENERATED: the Unicode 17.0.0 tables
  include/webcpp/idna/detail/properties.hpp  lookups over the tables
  include/webcpp/idna/detail/nfc.hpp         NFC normalization and quick check
  include/webcpp/idna/detail/bidi.hpp        RFC 5893
  include/webcpp/idna/detail/context.hpp     RFC 5892 appendix A (CONTEXTJ, CONTEXTO)
  include/webcpp/idna/detail/label_buffer.hpp  inline working storage, pmr beyond
  include/webcpp/idna/detail/process.hpp     the label driver shared by uts46 and validity
  tools/generate_tables.py               the generator (section 8.3)
  tools/unicode.json                     the pinned inputs: URLs and SHA-256
  test/Jamfile  test/*_test.cpp  test/require.hpp
  test/generated/idna_test_v2.ipp        GENERATED: IdnaTestV2.txt as data
  test/generated/nfc_test.ipp            GENERATED: NormalizationTest.txt's NFC column
  test/known_failures.json               tracked known failures (expected empty)
  example/Jamfile  example/idna_*.cpp  example/idna_*.expected
  doc/Jamfile  doc/mrdocs.yml  doc/idna.adoc (+ section files)  doc/counts.py
  README.md  AGENTS.md  LICENSE_1_0.txt  .gitignore  .gitattributes
  .github/workflows/ci.yml               library.yml with library: idna
```

`AGENTS.md` of idna holds only what is specific: the Unicode version and how
to move to the next one (section 8.4), that ada-url is read for algorithms
and never copied, and the strict profile's reason.

### 3.3 `build.jam`, `meta/`

```
# Copyright (c) 2026 WebCpp.org
# ... the licence notice
#
# idna, IDNA2008 and UTS 46 for C++20. A program uses it through
# /webcpp/idna//idna, which adds its headers and Boost's.

project /webcpp/idna ;

alias idna : : : : <include>include <library>/boost//headers ;
```

- `meta/libraries.json`: key and name `idna`, authors `["R"]`,
  maintainers as xactor's, description "IDNA2008 and UTS 46 for C++20:
  Punycode, ToASCII and ToUnicode, and label validity, over generated
  Unicode 17.0 tables.", category `["String"]`, `"cxxstd": "20"`,
  `"port-of": null`.
- `meta/include-boundaries.json`: every header under `include/` must not
  include `boost/url`, `boost/json`, `srell` or `webcpp/formats`, "idna
  depends on webcpp::config and header-only Boost alone".

### 3.4 Dependencies, and the config migration

Of webcpp, `webcpp::config` only. Of Boost, header-only libraries every
webcpp library uses: Config, System (`error_code`, `result`), Core, Assert.
Nothing compiled, nothing third-party. `<memory_resource>` and `<span>` from
the standard library.

Today `config.hpp` reads `#include <boost/config.hpp>`. When `webcpp::config`
exists, that line becomes `#include <webcpp/config.hpp>`, and `build.jam`
adds `<library>/webcpp/config//config` to the alias: one line in each file,
in the same commit, with a changelog line.

## 4. Public API (the contract)

Every symbol below is frozen once I0 to I5 land: a change after that is a
change of the contract, announced to the formats track. Everything in
`detail` is named for orientation only and may change freely.

### 4.1 `<webcpp/idna/errors.hpp>`

```cpp
namespace webcpp::idna {

enum class error : int {
    invalid_utf8 = 1,                // the input is not well-formed UTF-8 (RFC 3629)
    disallowed = 2,                  // UTS 46 V7: a status other than valid (or deviation)
    not_idna2008 = 3,                // NV8/XV8: DISALLOWED in IDNA2008 (options::idna2008)
    not_ldh = 4,                     // UTS 46 U1: ASCII outside [a-z0-9-] (use_std3_ascii_rules)
    not_nfc = 5,                     // UTS 46 V1, RFC 5891 5.4
    hyphen_in_third_and_fourth = 6,  // UTS 46 V2 (check_hyphens)
    hyphen_at_start_or_end = 7,      // UTS 46 V3 (check_hyphens)
    ace_prefix = 8,                  // UTS 46 V4: "xn--" while check_hyphens is off
    leading_combining_mark = 9,      // UTS 46 V6, RFC 5891 5.4
    contextj = 10,                   // RFC 5892 A.1, A.2 (check_joiners); IdnaTestV2 C1, C2
    contexto = 11,                   // RFC 5892 A.3 to A.9 (idna2008)
    bidi = 12,                       // RFC 5893 rules 1 to 6 (check_bidi); IdnaTestV2 B1 to B6
    invalid_punycode = 13,           // RFC 3492 decoding, UTS 46 P4, a non-canonical A-label
    punycode_overflow = 14,          // RFC 3492 6.4; IdnaTestV2 A3
    empty_label = 15,                // UTS 46 A4_2, X4_2 (verify_dns_length)
    label_too_long = 16,             // more than 63 octets in ASCII form, A4_2
    domain_too_long = 17,            // more than 253 octets in ASCII form, A4_1
    output_too_small = 18,           // the caller's span cannot hold the result
    storage_exhausted = 19,          // a label beyond the inline working storage, no resource
};

const boost::system::error_category& error_category() noexcept;   // name "webcpp.idna"
boost::system::error_code make_error_code(error e) noexcept;

template <class T>
using result = boost::system::result<T>;

}  // namespace webcpp::idna

template <>
struct boost::system::is_error_code_enum<webcpp::idna::error> : std::true_type {};
```

The category follows pratt's form: a `constexpr` category object with the
64-bit identity of "idna" in ASCII, `0x69646E6100000000`, no mutable state.
Values are fixed and never reused.

### 4.2 `<webcpp/idna/options.hpp>`

```cpp
namespace webcpp::idna {

/** The flags of UTS 46 section 4, and IDNA2008's stricter validity. */
struct options {
    bool check_hyphens = true;            // CheckHyphens: V2, V3 (V4 when false)
    bool check_bidi = true;               // CheckBidi: RFC 5893 over a Bidi domain name
    bool check_joiners = true;            // CheckJoiners: RFC 5892 A.1, A.2
    bool use_std3_ascii_rules = true;     // UseSTD3ASCIIRules: U1
    bool verify_dns_length = true;        // VerifyDnsLength: A4_1, A4_2; no root label
    bool transitional_processing = false; // Transitional_Processing, deprecated by UTS 46
    bool ignore_invalid_punycode = false; // IgnoreInvalidPunycode
    bool idna2008 = true;                 // NV8/XV8 refused, and CONTEXTO (RFC 5892 A.3 to A.9)
};

inline constexpr std::size_t max_label_length = 63;    // octets of a label in ASCII form
inline constexpr std::size_t max_domain_length = 253;  // octets of a name, no root label
inline constexpr std::string_view unicode_version = "17.0.0";

}  // namespace webcpp::idna
```

`options` is an aggregate: a caller writes
`idna::options{.idna2008 = false}`.

### 4.3 `<webcpp/idna/punycode.hpp>`

```cpp
namespace webcpp::idna::punycode {

// RFC 3492 section 6.3: code points to Punycode (no "xn--" prefix). Returns
// the number of chars written. Errors: punycode_overflow, output_too_small,
// invalid_utf8 never (the input is code points; a surrogate or a value above
// U+10FFFF is invalid_punycode).
[[nodiscard]] result<std::size_t> encode(std::span<char> output,
                                         std::u32string_view input) noexcept;

// RFC 3492 section 6.2: Punycode (no prefix) to code points. Digits are read
// case-insensitively; basic code points are copied as they are, case
// included (RFC 3492 section 5, mixed-case annotation). Errors:
// invalid_punycode, punycode_overflow, output_too_small.
[[nodiscard]] result<std::size_t> decode(std::span<char32_t> output,
                                         std::string_view input) noexcept;

// The same, into a string of the given resource.
[[nodiscard]] result<std::pmr::string> encode(
    std::u32string_view input,
    std::pmr::memory_resource& resource = *std::pmr::get_default_resource());
[[nodiscard]] result<std::pmr::u32string> decode(
    std::string_view input,
    std::pmr::memory_resource& resource = *std::pmr::get_default_resource());

}  // namespace webcpp::idna::punycode
```

### 4.4 `<webcpp/idna/validity.hpp>`

IDNA2008 validity of what is given, **without mapping** (RFC 5891 section
5.4 and RFC 5892, as the UTS 46 validity criteria phrase them):

```cpp
namespace webcpp::idna {

// One label, in UTF-8: a U-label, an A-label ("xn--" in any ASCII case,
// decoded, validated, re-encoded and compared case-insensitively), or an
// NR-LDH label. The Bidi rule is applied to the label alone, as to a domain
// of one label.
[[nodiscard]] result<void> check_label(std::string_view label,
                                       const options& opts = {}) noexcept;

// A domain whose labels are separated by U+002E only: every label as
// check_label, the Bidi rule across all labels (RFC 5893 section 2, "Bidi
// domain name"), and the lengths when opts.verify_dns_length.
[[nodiscard]] result<void> check_domain(std::string_view domain,
                                        const options& opts = {}) noexcept;

[[nodiscard]] bool is_valid_label(std::string_view label, const options& opts = {}) noexcept;
[[nodiscard]] bool is_valid_domain(std::string_view domain, const options& opts = {}) noexcept;

}  // namespace webcpp::idna
```

`is_valid_*` is `check_*(...).has_value()`. None of the four allocates; a
label beyond the inline working storage (only with `verify_dns_length` off in `to_ascii` and the validators; `to_unicode`, which has no length check, meets it with the defaults: its span form stops there with `storage_exhausted`, after any earlier label's error, and its pmr form converts the label; amended 2026-10-09 after the UTS 46 review)
is `error::storage_exhausted` (P3).

### 4.5 `<webcpp/idna/uts46.hpp>`

UTS 46 processing **with mapping** (section 4 of UTS 46, then 4.2 ToASCII or
4.3 ToUnicode):

```cpp
namespace webcpp::idna {

// UTS 46 4.2 ToASCII into the caller's span: the number of chars written, or
// the first error. With verify_dns_length (the default) the result never
// exceeds max_domain_length, so a std::array<char, max_domain_length> always
// suffices.
[[nodiscard]] result<std::size_t> to_ascii(std::span<char> output, std::string_view domain,
                                           const options& opts = {}) noexcept;

// The same into a string of the given resource, which also provides the
// working storage of a label beyond the inline capacity.
[[nodiscard]] result<std::pmr::string> to_ascii(
    std::string_view domain, const options& opts = {},
    std::pmr::memory_resource& resource = *std::pmr::get_default_resource());

/** What to_unicode wrote, and the first error UTS 46 recorded, if any. */
struct unicode_result {
    std::size_t size = 0;              // chars written into the span
    boost::system::error_code error;   // empty when the processing succeeded
};

// UTS 46 4.3 ToUnicode: always converts as much as it can (UTS 46: "Even if an
// error occurs, the conversion of the string is performed as much as is
// possible"); a label that fails keeps its input form; no U+FFFD is inserted.
// output_too_small stops the writing at a code point boundary.
[[nodiscard]] unicode_result to_unicode(std::span<char> output, std::string_view domain,
                                        const options& opts = {}) noexcept;

/** The string to_unicode produced, and the first error UTS 46 recorded, if any. */
struct unicode_string {
    std::pmr::string value;
    boost::system::error_code error;
};

[[nodiscard]] unicode_string to_unicode(
    std::string_view domain, const options& opts = {},
    std::pmr::memory_resource& resource = *std::pmr::get_default_resource());

}  // namespace webcpp::idna
```

What formats calls (the contract it codes against):

| formats needs | It calls |
| --- | --- |
| `is_idn_hostname(s)` | `idna::to_ascii(buffer, s)` into a `std::array<char, idna::max_domain_length>`, default options: valid exactly when it has a value |
| an A-label of `is_hostname(s)` | `idna::check_label(label)` on each `xn--` label (case-insensitive prefix), lowercased into a 63-octet stack buffer first: the verdict idn-hostname gives that label, since UTS 46's mapping of an ASCII label is its lowercasing |
| `idna_label_rule` | `idna::to_ascii(buffer, label)` on one label (no separator inside) |
| the error of a failing IDN rule | the `idna::error` of the call, returned as the rule's error |

### 4.6 `<webcpp/idna.hpp>` and `config.hpp`

`<webcpp/idna.hpp>` includes every public header; nothing is optional or
heavy enough to leave out. `config.hpp` is pratt's, renamed:
`WEBCPP_IDNA_NO_EXCEPTIONS` defined automatically under
`BOOST_NO_EXCEPTIONS`, definable by the developer, defined and undefined at
once under `__MRDOCS__` so the reference lists it; its Doc Comment says idna
raises no exception of its own.

### 4.7 `detail` (named, not frozen)

- `detail::utf8`: `decode(std::string_view, std::size_t& i, char32_t& cp)`
  strict per RFC 3629 (no overlongs, surrogates, values above U+10FFFF,
  truncation), and `encode(char32_t, char*)`.
- `detail::tables`: the generated arrays (section 5.1).
- `detail::properties`: `uts46_status(cp)` (status, NV8/XV8 mark, mapping
  slice), `bidi_class(cp)`, `joining_type(cp)`, `script_of(cp)` (Greek,
  Hebrew, Hiragana, Katakana, Han, other), `combining_class(cp)`,
  `is_mark(cp)`, `nfc_quick_check(cp)`.
- `detail::nfc`: `normalize(span<char32_t>)` in place within a
  `label_buffer`, `is_nfc(u32string_view)`.
- `detail::bidi`, `detail::context`: the rule checks over a label's code
  points.
- `detail::label_buffer`: 256 inline code points, then a
  `std::pmr::vector<char32_t>` of a resource when one is given.
- `detail::process`: the per-label driver both `uts46.hpp` and
  `validity.hpp` run, with a sink that writes ASCII, Unicode or nothing.

## 5. Algorithms and rules

### 5.1 The tables (`detail/tables.hpp`, generated)

Generated from the Unicode 17.0.0 files of section 8.3, `constexpr` arrays of
fixed-width integers in read-only data, `inline` so a program holds one copy;
no inflation, no heap, no static mutable state. Each property is a sorted run
list (`{first code point, value}`) searched by binary search, the encoding
IDNA_SPIKE section 5 measured at 125,643 bytes raw for everything; a property
whose lookups dominate a measured profile may move to a two-stage table, a
detail change.

| Table | From | Used by |
| --- | --- | --- |
| UTS 46 status (valid, mapped, ignored, deviation, disallowed), NV8/XV8 mark, mapping index | `IdnaMappingTable.txt` | mapping, V7, P5's IDNA2008 property |
| mapping targets, UTF-32, with offsets | `IdnaMappingTable.txt` | mapping |
| Bidi_Class (L, R, AL, AN, EN, ES, CS, ET, ON, BN, NSM, other) | `extracted/DerivedBidiClass.txt` | RFC 5893 |
| Joining_Type (L, D, R, T, other) | `extracted/DerivedJoiningType.txt` | RFC 5892 A.1 |
| Canonical_Combining_Class | `extracted/DerivedCombiningClass.txt` | NFC; virama (ccc 9) for A.1, A.2 |
| General_Category Mark (Mn, Mc, Me) | `UnicodeData.txt` | V6 |
| Script: Greek, Hebrew, Hiragana, Katakana, Han | `Scripts.txt` | RFC 5892 A.4 to A.7 |
| canonical decompositions, primary composites | `UnicodeData.txt`, `DerivedNormalizationProps.txt` (Full_Composition_Exclusion) | NFC |
| NFC_Quick_Check (Yes, No, Maybe) | `DerivedNormalizationProps.txt` | `is_nfc` fast path |

Hangul syllables are decomposed and composed algorithmically (Unicode 17.0.0
section 3.12) and take no table space.

### 5.2 Punycode (RFC 3492)

Section 6.2 (decoding) and 6.3 (encoding) with the parameters of section 5
(base 36, tmin 1, tmax 26, skew 38, damp 700, initial_bias 72, initial_n
128), the bias adaptation of 6.1, and the overflow handling of 6.4 with
32-bit unsigned arithmetic (`punycode_overflow`). Decoding: the basic code
points are those before the last delimiter, copied as they are (a
non-basic one is `invalid_punycode`); a delimiter at position 0 consumes
nothing (so `xn---9uc` fails as RFC 3492 says); every other character must be
a digit in either case; a decoded value above U+10FFFF or in the surrogate
range is `invalid_punycode`.

### 5.3 UTS 46 processing (`to_ascii`, `to_unicode`)

Revision 35, section 4, streamed **label by label** so that the working
storage is one label (P3):

1. **Decode** the input as UTF-8 (`invalid_utf8` at the first ill-formed
   sequence; UTS 46 works on code points, so ill-formed input is refused
   before any processing).
2. **Map** each code point by its status: `disallowed` kept (refused at
   step 5), `ignored` removed, `mapped` replaced by its target, `deviation`
   replaced by its target only with `transitional_processing`, `valid` kept.
   U+3002, U+FF0E and U+FF61 map to U+002E like any mapped code point; the
   label ends at each U+002E of the mapped stream.
3. **Normalize** the label to NFC (section 5.5).
4. **Convert**: a label that starts with `xn--` must be all ASCII
   (`invalid_punycode`), is decoded (section 5.2; `invalid_punycode` unless
   `ignore_invalid_punycode`), and the decoded label must be non-empty and
   hold a non-ASCII code point (`invalid_punycode`, UTS 46 P4).
5. **Validate** (UTS 46 4.1, nontransitional for a decoded label): V1 NFC,
   V2 and V3 hyphens (or V4 when `check_hyphens` is off), V6 leading mark, V7
   status (valid, or deviation when nontransitional), U1 under
   `use_std3_ascii_rules`, CONTEXTJ under `check_joiners` (section 5.6), and,
   under `idna2008`, no NV8/XV8 code point (`not_idna2008`) and CONTEXTO
   (section 5.6).
6. **Bidi** under `check_bidi`: each label records whether it is an RTL label
   (RFC 5893's "RTL label": it holds an R, AL or AN code point, which makes the domain a Bidi domain name) and whether it satisfies the rules that apply to it
   (section 5.7); once every label is seen, a domain that holds an RTL label
   is a Bidi domain name and fails if any label failed its rules.
7. **ToASCII** (4.2): a label with a non-ASCII code point is encoded
   (section 5.2) and prefixed with `xn--`; then, under `verify_dns_length`,
   every label is 1 to 63 octets, the name 1 to 253 octets, and the empty
   root label (a trailing dot) is refused, as revision 35 states ("When
   VerifyDnsLength is true, the empty root label is disallowed").
8. **ToUnicode** (4.3): each label is written in its Unicode form (decoded
   A-labels included); an error is recorded and the conversion goes on.

### 5.4 IDNA2008 validity without mapping (`check_label`, `check_domain`)

The input is taken as the labels it is, split at U+002E only. Each label runs
steps 4 to 6 of section 5.3 on its own code points (no map, no normalize, so
a label not in NFC is `not_nfc`, RFC 5891 section 5.4); an A-label is also
re-encoded and compared with its input, ASCII case-insensitively, so a
non-canonical A-label is `invalid_punycode` (RFC 5891 section 5.4). Lengths
and the Bidi domain rule are those of steps 6 and 7.

### 5.5 NFC (Unicode 17.0.0 section 3.11, UAX 15)

Canonical decomposition (recursive, Hangul algorithmic), canonical ordering
by combining class (a stable insertion sort within the label buffer), then
canonical composition with primary composites only (Full_Composition_Exclusion
excluded). `is_nfc` answers Yes or No by NFC_Quick_Check, and for Maybe
normalizes a copy within the label buffer and compares.

### 5.6 Contextual rules (RFC 5892 appendix A)

| Rule | Code points | Valid when |
| --- | --- | --- |
| A.1 ZERO WIDTH NON-JOINER | U+200C | the code point before is a virama (ccc 9), or the label matches `(Joining_Type:{L,D})(Joining_Type:T)*‌(Joining_Type:T)*(Joining_Type:{R,D})` around this occurrence; checked at every occurrence |
| A.2 ZERO WIDTH JOINER | U+200D | the code point before is a virama |
| A.3 MIDDLE DOT | U+00B7 | between two U+006C |
| A.4 GREEK KERAIA | U+0375 | the next code point is Greek |
| A.5, A.6 HEBREW GERESH, GERSHAYIM | U+05F3, U+05F4 | the code point before is Hebrew |
| A.7 KATAKANA MIDDLE DOT | U+30FB | the label holds a Hiragana, Katakana or Han code point |
| A.8 ARABIC-INDIC DIGITS | U+0660 to U+0669 | the label holds no U+06F0 to U+06F9 |
| A.9 EXTENDED ARABIC-INDIC DIGITS | U+06F0 to U+06F9 | the label holds no U+0660 to U+0669 |

The CONTEXTJ and CONTEXTO code point sets are these, fixed by RFC 5892;
IANA's 12.0.0 table agrees (P5's comparison checks it).

### 5.7 The Bidi rule (RFC 5893 section 2)

1. The first code point is L, R or AL. 2. In an RTL label, only R, AL, AN,
EN, ES, CS, ET, ON, BN, NSM. 3. An RTL label ends with R, AL, EN or AN,
then zero or more NSM. 4. An RTL label does not hold both EN and AN. 5. In an
LTR label, only L, EN, ES, CS, ET, ON, BN, NSM. 6. An LTR label ends with L
or EN, then zero or more NSM. The rules' direction is RTL when the label's first code point is R or AL, else LTR (rule 1); this is not section 5.3 step 6's "RTL label", which holds R, AL or AN anywhere. In a Bidi domain name every label satisfies rule 1 and the rules of its direction (so `0a.א` fails rule 1 in its first label).

## 6. Errors

- Every fallible operation returns `result<T>` or, for `to_unicode`, a
  struct carrying an `error_code`, of the category `webcpp.idna`; nothing
  throws. The first error found is the one reported: labels are processed in
  order, and within a label the checks run in the order of section 5.3.
- `noexcept` holds for every span form and validator. The pmr forms may
  propagate what their resource throws (the default resource throws
  `std::bad_alloc`; in a build without exceptions it aborts, as
  `tools/throw_exception.cpp` does); they raise nothing of their own.
- No API is throwing, so nothing sits behind `WEBCPP_IDNA_NO_EXCEPTIONS`,
  whose Doc Comment says so (AGENTS chapter 6: a library that raises no
  exception of its own still has the macro). The lint's bare-throw rule and
  the no-exceptions twin of every compile hold idna to it.

## 7. Allocation

| Operation | Allocates |
| --- | --- |
| `punycode::encode`/`decode` into a span | never |
| `check_label`, `check_domain`, `is_valid_*` | never |
| `to_ascii`, `to_unicode` into a span | never; a label beyond 256 code points is `storage_exhausted` (in `to_ascii` only with `verify_dns_length` off; in `to_unicode` with any options, section 4.4) |
| the pmr forms | only from the given resource: the returned string, and a label's working storage beyond the inline capacity |

Inline working storage: one `label_buffer` of 256 `char32_t` (1 KiB) and the
63-octet ASCII form of a label, on the stack, well inside wasip2's default
64 KiB stack (pratt's measurement of the default stack). Proof: an
allocation test replaces `operator new` and `operator delete` for itself and
counts zero across every span form and validator on long and hostile inputs,
and counts, through a counting `memory_resource`, that every allocation of a
pmr form goes through its resource.

## 8. The build

### 8.1 Targets and Jamfiles

- `build.jam` as in section 3.3. Header-only; no compiled dependency.
- `test/Jamfile`: `webcpp.targets native wasip2 wasip3 emscripten ;`,
  `webcpp.headers-alone idna : ../include ;`, `config` compiled twice (as
  built, and with `<define>WEBCPP_IDNA_NO_EXCEPTIONS`), and one `webcpp.run`
  per test of section 9. MSVC gets `<toolset>msvc:<cxxflags>/bigobj` on the
  tests that include the generated vectors.
- The own lane `tables` (no target), declared in `test/Jamfile` with S1's
  rule, which re-runs the generator and compares (section 8.3).
- `example/Jamfile` and `doc/Jamfile` as AGENTS chapters 8 and 9 say.

### 8.2 Compile cost

The generated header is the heaviest include of the library (about 126 KB of
data as integer literals). Only `detail/properties.hpp` includes it, and only
`uts46.hpp` and `validity.hpp` include that; `punycode.hpp`, `errors.hpp` and
`options.hpp` do not. I2 measures a translation unit including `<webcpp/idna.hpp>`
(compile time and object size, native and wasip2) and records it on the page;
above 2 s at `-O2` natively, the tables move to a string-literal encoding
decoded by `constexpr` accessors, a detail change.

### 8.3 The generator and its check

`tools/generate_tables.py`, Python 3.9 or newer, standard library only,
Pyright-clean, 100 columns, the licence notice (AGENTS chapter 11). About 600
to 800 lines.

- **Inputs** are pinned in `tools/unicode.json`, each a URL under
  `https://www.unicode.org/Public/17.0.0/` and its SHA-256 (downloaded and
  hashed 2026-10-09):

  | File | SHA-256 |
  | --- | --- |
  | `idna/IdnaMappingTable.txt` | `87f05505dc026fdb2bff16132bdc68a8014675836882a9a2b1844540ad3be382` |
  | `idna/IdnaTestV2.txt` | `beb5d0be20e896189b03209a82fdc34f06351502bbd4b8e2523583fc2954d9cf` |
  | `ucd/UnicodeData.txt` | `2e1efc1dcb59c575eedf5ccae60f95229f706ee6d031835247d843c11d96470c` |
  | `ucd/extracted/DerivedBidiClass.txt` | `4867b4b7f0731ed1bfcd34cc6251211ff1542541fce0734b6fbda139ee80b3a4` |
  | `ucd/extracted/DerivedJoiningType.txt` | `f39ebe974825d6736aee15582250307aa532b2cfab3caf3f86bd23fddc9c5c4d` |
  | `ucd/extracted/DerivedCombiningClass.txt` | `191463abfbd202703c6fd6776a92a23ac44ec65e0476a7f95aa91ca492cef29b` |
  | `ucd/DerivedNormalizationProps.txt` | `71fd6a206a2c0cdd41feb6b7f656aa31091db45e9cedc926985d718397f9e488` |
  | `ucd/Scripts.txt` | `9f5e50d3abaee7d6ce09480f325c706f485ae3240912527e651954d2d6b035bf` |
  | `ucd/NormalizationTest.txt` | `5019ffd530751a741900c849c0e010332f142a3612234639bd200b82138a87db` |

  plus IANA's `https://www.iana.org/assignments/idna-tables-12.0.0/idna-tables-properties.csv`,
  `160b72e33a377a84253393c572dc338879f39b93beccbb9cb9baebbf4d6dd743`, read
  only by the self-check below.
- **Fetching**: `--cache <dir>` (default `bin/unicode/17.0.0/` of the
  superproject) holds the files; a missing one is downloaded, every one is
  hashed, and a mismatch stops the generator naming the file and both
  digests, writing nothing.
- **Outputs**: `include/webcpp/idna/detail/tables.hpp`,
  `test/generated/idna_test_v2.ipp` (6,391 vectors) and
  `test/generated/nfc_test.ipp` (the NFC column of 20,034 lines), each opened
  by the licence notice, a line naming the generator, the Unicode version and
  "do not edit", and P8's notice line; the data between
  `// clang-format off` and `// clang-format on`, with
  `NOLINTBEGIN(readability-magic-numbers)` and its reason if clang-tidy asks;
  every `detail` symbol with its one-sentence Doc Comment. Deterministic: no
  date, sorted keys, `\n` endings; two runs write the same bytes.
- **Self-checks**, each stopping the generator by name: every status of
  `IdnaMappingTable.txt` is one of the five known ones; every code point
  P5 derives agrees with IANA 12.0.0 but U+002E; the CONTEXTJ and CONTEXTO
  sets of section 5.6 are those IANA lists; no full canonical decomposition
  exceeds 4 code points (P3's bound).
- **The check** (S1's rule, own lane `tables`): runs the generator into the
  build directory and compares each output with the committed file byte for
  byte; `update-generated` writes them in place, the only writer of
  generated files, as `update-expected` is for an oracle (AGENTS chapter 5).
  It runs on demand and in CI's own-lane job (which has the network the
  downloads need); the ordinary lanes never run it.

### 8.4 Moving to a new Unicode version

One change of its own: update `tools/unicode.json` (version, URLs,
digests) and `options.hpp`'s `unicode_version`, run `update-generated`, run
every lane, update the known failures (each with its reason) and the page's
version attribute, and add a changelog line. Unicode 18.0.0 is published
(UTS 46 revision 36, 2026-08-31); the move is not in scope here.

## 9. Tests

Unit tests use lightweight_test, one `webcpp.run` each, on native, wasip2,
wasip3 and emscripten; every vector is compiled in, so no test reads a file.

| Test | What it proves |
| --- | --- |
| `config` (twice) | the macro is defined exactly when it should be |
| `errors_test` | every enumerator's value and message, the category's name and identity, `make_error_code` |
| `punycode_test` | RFC 3492 section 7.1's 19 samples, (A) to (S), both ways; case-insensitive digits; the mixed-case annotation kept; a leading delimiter; invalid digits; an overflowing input; a value above U+10FFFF; `output_too_small` at every length below the result's |
| `utf8_test` | RFC 3629's table of well-formed sequences, and every class of ill-formed one: truncated, overlong, surrogate, above U+10FFFF, a lone continuation byte |
| `tables_test` | spot properties: U+00A1 `valid NV8`, U+0041 mapped to U+0061, U+00AD ignored, U+00DF deviation, U+05D0 R, U+0627 AL and Joining_Type R, U+094D ccc 9, U+0375 and U+03B2 Greek, U+30FB not Katakana, U+0300 a mark |
| `nfc_test` | `normalize` gives column 2 from columns 1 to 3 of all 20,034 lines of `NormalizationTest.txt`, and `is_nfc` agrees with it |
| `validity_test` | each rule of section 5.6 and 5.7, valid and invalid, from RFC 5892 appendix A and RFC 5893 section 2; the 15 strings ada fails (IDNA_SPIKE section 2.3) as U-labels and as A-labels; NR-LDH, R-LDH and A-label edge cases; the lengths at 63/64 and 253/254 |
| `uts46_test` | all 6,391 lines of `IdnaTestV2.txt`: toUnicode, toAsciiN and toAsciiT (`transitional_processing`), comparing the strings exactly and the presence of an error, with `idna2008 = false` and every other flag on, as the file assumes; then the strict pass with `idna2008 = true` over the lines whose toUnicode field holds no NV8/XV8 code point and no CONTEXTO code point (the file says an IDNA2008 implementation skips the former, and does not test the latter) |
| `allocation_test` | section 7 |
| `headers-alone idna` | every public header compiles alone |

**RFC examples** are those named above; **the JSON Schema suite's** `hostname`
and `idn-hostname` files run in `webcpp::formats` (its section 9), and every
failure there that points to idna becomes a `validity_test` or `uts46_test`
case here, red first.

**100%, and known failures.** `test/known_failures.json` lists IdnaTestV2
lines (by line number in the pinned file and source string) with a reason
each; `uts46_test` fails on a failure the list does not name and on a listed
line that passes, so the list never lies. Expected empty: IdnaTestV2 is
written for the same Unicode version as the tables. A residue that comes
only from a Unicode version difference (a later suite or a later Unicode) is
the one kind of entry allowed, and its reason names both versions.

## 10. Documentation

- Doc Comments on every public symbol (AGENTS chapter 7): a one-sentence
  brief, `@param`, `@tparam`, `@return`, `@note`, `@see "<title>"`; each
  enumerator and data member its own comment; `detail` symbols a brief. The
  MrDocs reference is strict; `doc/mrdocs.yml` sets `show-enum-constants:
  true` only.
- `doc/idna.adoc`, xactor's page as the model: an introduction (IDNA2008,
  UTS 46 and Punycode, and why both standards: the strict profile, with a
  first example); Punycode; converting names (`to_ascii`, `to_unicode`, the
  options and their UTS 46 names, the span and pmr forms); validity (labels,
  domains, each rule with its RFC section); errors (the table of section
  4.1, each enumerator with the UTS 46 status code it reports); allocation;
  limits (lengths, the inline storage, `storage_exhausted`); the Unicode
  version and the tables (the generator, updating Unicode); guarantees;
  testing; the reference; an appendix with the Unicode data notice (P8).
- Guarantees, numbered and anchored (`idna-invariant-<n>`): (1) no operation
  of the span forms or the validators allocates; (2) nothing throws but a
  resource's allocation; (3) the same input gives the same output on every
  target; (4) `to_ascii` with `verify_dns_length` writes at most 253 octets;
  (5) the tables are exactly what the generator writes from the pinned
  Unicode 17.0.0 files.
- Examples, each compiled and run on every target with its `.expected`:
  `idna_quick_start.cpp` (`to_ascii` and `to_unicode` of `bücher.example`),
  `idna_punycode.cpp` (RFC 3492 sample), `idna_validity.cpp` (`check_label`
  and the reason of a CONTEXTO failure), `idna_allocation.cpp` (a span form,
  then a pmr form on a `monotonic_buffer_resource`).
- `doc/counts.py` counts what only the page holds: `{n-idna-test-v2}`,
  `{n-nfc-vectors}`, from the generated files.

## 11. The changelog

Each increment's report carries a "Changelog" section for idna, in Boost's
style; the lines go into idna's `doc/changelog.adoc` under "Unreleased" once
the release design's changelog files exist, and into
`~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md` until then
(`WEBCPP_RELEASE_DESIGN.md` decision 6). I0 writes the library's line, under
"New Libraries":

- idna: IDNA2008 and UTS 46 for C++20, with Punycode and generated Unicode
  17.0 tables, from R.

Until the first release, later increments refine that line rather than add
bullets (a new library is one line in Boost's notes); after it, every
user-visible change adds a bullet (Added, Changed, Fixed, *Breaking
change:*).

## 12. Increments

Each ends with the named tests passing from scratch (`b2 -a`) on every
declared lane, the lint clean, the page building, and a commit. **∥** marks
what can run in parallel with what.

**Superproject prerequisite (shared with json_schema, built once).**

- **S1. The check of generated files.** A rule `webcpp.generated <name> :
  <script> : <arguments> * : <outputs> +` in `tools/webcpp.jam`: the target
  `generated-<name>`, which runs the script into the build directory and
  compares each output with the committed file (`tools/oracle/compare.py`),
  and `update-generated`, which writes them in place; a lane names it with
  `webcpp.lane`. Tests in `tools/test/` red first (a matching output, a
  differing one, a missing one, `update-generated` writing). ∥ I0, I1. Needed
  by I2; json_schema's `.pot` check uses it too.

**idna.**

- **I0. Skeleton.** The tree of section 3.2 with an empty convenience header,
  `config.hpp`, `errors.hpp`, `options.hpp`; `build.jam`, `meta/`, CI file,
  README, AGENTS.md; the page with its introduction and the reference.
  Passing: `headers-alone`, `config` (twice), `errors_test`, on all four
  targets. ∥ S1, formats F0.
- **I1. Punycode.** `punycode.hpp`, `detail/utf8.hpp`. Passing:
  `punycode_test`, `utf8_test`, the punycode part of `allocation_test`. ∥ I2.
- **I2. Generator and tables.** `tools/`, `detail/tables.hpp`,
  `detail/properties.hpp`, the generated vectors, the own lane `tables`.
  Needs S1 and P8's decision. Passing: `tables_test` on all four targets; the
  `tables` lane; the compile-cost measurement recorded. ∥ I1.
- **I3. NFC.** `detail/nfc.hpp`, `detail/label_buffer.hpp`. Needs I2.
  Passing: `nfc_test` (all 20,034 lines). ∥ I1 if it is still running.
- **I4. Validity.** `validity.hpp`, `detail/context.hpp`, `detail/bidi.hpp`,
  `detail/process.hpp`. Needs I1, I3. Passing: `validity_test`; the validity
  part of `allocation_test`. Unblocks formats F5.
- **I5. UTS 46.** `uts46.hpp`. Needs I4. Passing: `uts46_test` at 100%
  (the three columns and the strict pass), the whole `allocation_test`.
  Unblocks formats F6.
- **I6. Documentation and readiness.** The page complete (every section of
  section 10, the guarantees, the examples with their `.expected`, the
  counts), the README, the changelog line final. Passing: `b2 -a
  libs/idna/test libs/idna/example` on the four targets, `b2 -a
  libs/idna/doc`, the `tables` lane, the lint. ∥ formats F6, F7.

## 13. Risks

1. **The Unicode data notice (P8).** The generated tables and vectors derive
   from Unicode's data files; the Unicode License v3 asks for its notice with
   the copies or in the documentation, and AGENTS says every file of webcpp is
   BSL-1.0 only. Needs the owner's decision before I2 commits generated data;
   the proposal keeps the code BSL-1.0 and puts the notice in the page.
2. **Compile time** of the 126 KB tables in every translation unit that
   includes `uts46.hpp` or `validity.hpp` (section 8.2). Measured in I2, with
   a fallback encoding.
3. **The generator check needs the network** (unicode.org and iana.org): it
   runs as an own lane, never in the ordinary ones; a cache directory makes it
   offline once filled. A moved URL fails the lane by name, never silently.
4. **UTS 46 is not IDNA2008.** The strict profile adds IDNA2008 to UTS 46;
   IdnaTestV2 does not test CONTEXTO, and nothing official tests the
   combination. The JSON Schema suite (through formats), RFC 5892's rules
   case by case, and the 15 strings of IDNA_SPIKE cover it; a differential
   run against libidn2's registration check (2.2% failing on the suite) is a
   local aid, never a dependency.
5. **P5 rests on a measurement** against IANA 12.0.0; code points assigned
   after Unicode 12.0 have no IANA property to compare with. RFC 5892's
   derivation is stable by design (its exceptions are fixed), and the
   generator's comparison runs on every regeneration.
6. **Stack use on wasm.** About 1.3 KiB of inline storage per call, measured
   in I5 on wasip2 at `-O0` against the 64 KiB default stack, as pratt
   measured its depth limit.
7. **The shared prerequisite S1** is superproject work json_schema needs
   too; whichever track reaches it first builds it, on a branch of its own,
   merged to `main` before I2.
8. **Unicode 18.0.0 is already out.** The owner chose 17.0; SRELL, which
   formats' regex uses, carries Unicode 18.0 tables. The two never meet (regex
   property escapes and IDNA validity are separate questions), and the page
   names both versions.

Approved by the owner on 2026-10-09.
