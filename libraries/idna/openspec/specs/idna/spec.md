# idna Specification

## Purpose
webcpp::idna is a header-only C++20 library of webcpp's own for internationalized domain names:
Punycode (RFC 3492), UTS 46's ToASCII and ToUnicode (sections 4.2 and 4.3, revision 35), and the
validity rules of IDNA2008 (RFC 5890 to 5893), over tables generated from Unicode 17.0.0. Its
default profile is UTS 46's mapping followed by IDNA2008's validity. Every operation returns its
error as a value, the span forms and the validators allocate nothing, and it runs natively and on
wasm32-wasip2, wasm32-wasip3 and Emscripten. These requirements describe 0.1.0 as built
(`../../changes/archive/2026-10-09-genesis/reference/source/`).

## Requirements

### Requirement: Library: Header-only library in its namespace
The library SHALL be header-only C++20, in namespace `webcpp::idna`, with Punycode's functions in the nested namespace `webcpp::idna::punycode`, internals in `webcpp::idna::detail` (and its nested namespaces `detail::punycode`, `detail::utf8`, `detail::nfc`, `detail::context`, `detail::bidi` and `detail::tables`), public headers under `<webcpp/idna/...>` (one per responsibility), macros prefixed `WEBCPP_IDNA_` and include guards `WEBCPP_IDNA_<PATH>_HPP` (`WEBCPP_IDNA_HPP`, `WEBCPP_IDNA_CONFIG_HPP`, `WEBCPP_IDNA_DETAIL_PROCESS_HPP`). It ports nothing: `meta/libraries.json` has `"port-of": null`, and idna has no oracle, no twins and no "Differences" appendix; its conformance is Unicode's own test files.

#### Scenario: A program uses the library without building it
- **WHEN** a b2 program names `/webcpp/idna//idna` and includes `<webcpp/idna.hpp>`
- **THEN** it compiles and links without any library of idna's own being built, since every header is self-contained and the tables are `constexpr` data

#### Scenario: The library is not a port
- **WHEN** `meta/libraries.json` is read
- **THEN** it has `"key": "idna"`, `"name": "idna"`, `"cxxstd": "20"` and `"port-of": null`

### Requirement: Library: Public headers and the convenience header
The library SHALL ship these public headers under `include/webcpp/idna/`: `config.hpp`, `errors.hpp`, `options.hpp`, `punycode.hpp`, `validity.hpp` and `uts46.hpp`, and the convenience header `include/webcpp/idna.hpp`, which MUST include `config.hpp` first and then `errors.hpp`, `options.hpp`, `punycode.hpp`, `uts46.hpp` and `validity.hpp`. The names are in `webcpp::idna`, and Punycode's `encode` and `decode` in `webcpp::idna::punycode`. Every public header but `config.hpp` itself, and every header under `detail/`, MUST include `<webcpp/idna/config.hpp>` first. `punycode.hpp`, `errors.hpp` and `options.hpp` MUST NOT reach the tables; of the public headers, only `uts46.hpp` and `validity.hpp` (and so `idna.hpp`) include them.

#### Scenario: One include brings everything
- **WHEN** a program includes only `<webcpp/idna.hpp>` and calls `idna::to_ascii`, `idna::check_label` and `idna::punycode::encode`
- **THEN** it compiles, because the convenience header declares all of them

#### Scenario: Punycode alone costs no tables
- **WHEN** a program includes only `<webcpp/idna/punycode.hpp>`
- **THEN** `detail/tables.hpp` and `detail/properties.hpp` are not included

### Requirement: Library: Internal headers
The library SHALL keep its internals in eight headers under `include/webcpp/idna/detail/`, named but not frozen (they may change freely): `utf8.hpp` (strict UTF-8), `tables.hpp` (the generated Unicode 17.0.0 tables), `properties.hpp` (the lookups into them), `label_buffer.hpp` (a label's storage), `nfc.hpp` (Normalization Form C), `context.hpp` (RFC 5892 appendix A), `bidi.hpp` (RFC 5893 section 2) and `process.hpp` (the label driver `uts46.hpp` and `validity.hpp` share). Only `detail/properties.hpp` MUST include `detail/tables.hpp`, and it MUST refuse to compile (a `static_assert`) while `detail::tables::unicode_version` differs from `idna::unicode_version`. The public interface, by contrast, is a contract that webcpp::lexical codes against: the signatures of `errors.hpp`, `options.hpp`, `punycode.hpp`, `validity.hpp` and `uts46.hpp` MUST change only with an announcement to it.

#### Scenario: The tables and the options name one version
- **WHEN** `options.hpp`'s `unicode_version` and the tables' `unicode_version` differ
- **THEN** `detail/properties.hpp` fails to compile

#### Scenario: Only the properties header reads the tables
- **WHEN** the headers are searched for `#include <webcpp/idna/detail/tables.hpp>`
- **THEN** only `detail/properties.hpp` includes it

### Requirement: Library: Build target and dependencies
`build.jam` SHALL declare `project /webcpp/idna` and one alias `idna` whose usage requirements are `<include>include`, `<library>/boost//headers` and `<library>/webcpp/config//config`. idna needs Boost's headers (Config, System, Core and Assert; Boost 1.92 or newer) and webcpp's configuration, and nothing to build or link: no Boost.URL, no Boost.JSON, no SRELL, no compiled library of any kind. The tests link `/webcpp/idna//idna` through the requirements of `test/Jamfile` and `example/Jamfile`.

#### Scenario: One alias brings everything
- **WHEN** a program names `/webcpp/idna//idna`
- **THEN** idna's include path, Boost's headers and webcpp's configuration reach its compile line, and nothing is added to its link line

#### Scenario: Nothing is compiled
- **WHEN** `build.jam` is read
- **THEN** it declares an alias with empty sources and no `lib`, `obj` or `exe` target

### Requirement: Library: Include boundaries
`meta/include-boundaries.json` SHALL declare one boundary for `include/webcpp/idna.hpp`, `include/webcpp/idna/*.hpp` and `include/webcpp/idna/detail/*.hpp`: they must not include `boost/url`, `boost/json`, `srell` or `webcpp/formats`, with the reason "idna depends on webcpp::config and header-only Boost alone". The superproject's lint enforces it.

#### Scenario: A header includes Boost.URL
- **WHEN** a header of idna includes `<boost/url.hpp>`
- **THEN** the superproject's lint fails, naming the boundary

### Requirement: Library: Targets
`test/Jamfile` and `example/Jamfile` SHALL each declare `webcpp.targets native wasip2 wasip3 emscripten`, so that every test and every example is built and run on native (any toolset), wasm32-wasip2 (toolset `clang-wasip2`, launched by wasmtime), wasm32-wasip3 (toolset `clang-wasip3`, launched by wasmtime) and Emscripten (run under Node). Every test runs the same vectors on every target, which is how guarantee 3 (the same input and options give the same output and error on every target) is checked.

#### Scenario: The Jamfiles name the four targets
- **WHEN** `test/Jamfile` and `example/Jamfile` are read
- **THEN** each contains the line `webcpp.targets native wasip2 wasip3 emscripten ;`

### Requirement: Library: Configuration macro for exceptions
`<webcpp/idna/config.hpp>` SHALL include `<webcpp/config.hpp>` first and define `WEBCPP_IDNA_NO_EXCEPTIONS` when `WEBCPP_HAS_EXCEPTIONS` is not defined (the compiler has no exceptions, or the program defined `WEBCPP_NO_EXCEPTIONS` for every webcpp library) and MUST keep the macro when a program defines it for idna alone in a build that has exceptions. idna raises no exception of its own, so the macro changes nothing in it; it exists so that a program can set it for every webcpp library alike, and an API that threw would go through `boost::throw_exception` and be absent while it is defined. Under `__MRDOCS__` the header defines and undefines the macro at once so that the reference lists it.

#### Scenario: A build with exceptions
- **WHEN** `config_test.cpp` is compiled with webcpp's exceptions available and no macro of the test's
- **THEN** `WEBCPP_IDNA_NO_EXCEPTIONS` is not defined, since `defined(WEBCPP_HAS_EXCEPTIONS) != defined(WEBCPP_IDNA_NO_EXCEPTIONS)`

#### Scenario: All webcpp libraries switch exceptions off
- **WHEN** `config_test.cpp` is compiled with `-DWEBCPP_NO_EXCEPTIONS -DWEBCPP_TEST_IDNA_ALL_OFF`
- **THEN** `WEBCPP_IDNA_NO_EXCEPTIONS` is defined

#### Scenario: The developer defines the macro for idna alone
- **WHEN** `config_test.cpp` is compiled with `-DWEBCPP_IDNA_NO_EXCEPTIONS -DWEBCPP_TEST_IDNA_DEVELOPER_DEFINED` in a build that has exceptions
- **THEN** the macro is still defined after `config.hpp` is included

### Requirement: Library: No exceptions in a header
No header of idna SHALL contain a `throw`, `try` or `catch`, and the headers MUST compile without exceptions (the lint compiles every header, test and example a second time without them). A `noexcept` function MUST read a `result` it has checked with `unsafe_value()`, never with `operator*`, because Boost 1.92's `result::operator*` calls `value()`, which throws, and clang-tidy's `bugprone-exception-escape` then fails the function.

#### Scenario: Every header compiles without exceptions
- **WHEN** the lint compiles every header, test and example a second time without exceptions
- **THEN** it accepts them, `punycode::decode` and the validators included, which read their results with `unsafe_value()` after `has_error()` was checked

### Requirement: Library: Literals in sources
A narrow string literal of idna's sources SHALL spell a byte above 0x7F as a three-digit octal escape (`"b\303\274cher"`), because a hex escape runs into the letters after it and raw UTF-8 is read in the machine's code page by MSVC; a `U""` literal MUST use `\u` and `\U`, and `\x` only for a value no universal character name may name (a surrogate). A default `options` MUST be written `idna::options()`, because clang-tidy's `readability-trailing-comma` misreads an empty `options{}` followed by a comma.

#### Scenario: The quick start spells its names in octal
- **WHEN** `example/idna_quick_start.cpp` is read
- **THEN** it spells U+00FC as `\303\274` and U+00B7 as `\302\267`

### Requirement: Library: Repository files of a webcpp library
The repository SHALL hold `build.jam`, `meta/libraries.json` (key `idna`, name `idna`, `authors` `["R"]`, `maintainers` `["R <github.overboard312@passmail.net>"]`, `cxxstd` `"20"`, `"port-of": null`, category `["String"]`, and the description "IDNA2008 and UTS 46 for C++20: Punycode, ToASCII and ToUnicode, and label validity, over generated Unicode 17.0 tables."), `meta/include-boundaries.json`, `README.md`, `AGENTS.md` (only what is specific to idna, the superproject's rules applying), `LICENSE_1_0.txt`, `doc/` (the page, its sections, `mrdocs.yml`, `counts.py` and `Jamfile`), `tools/` (`generate_tables.py` and `unicode.json`), `test/`, `example/`, `.gitignore` (`doc/html/`), `.gitattributes` (`* text=auto eol=lf`, `*.bat text eol=crlf`) and `.github/workflows/ci.yml`, which calls the superproject's reusable workflow `webcpporg/webcpp/.github/workflows/library.yml@main` with `library: idna` on push to `main`, pull requests and manual dispatch, with `contents: read` permission and a concurrency group that never cancels a run on `main`.

#### Scenario: The CI is the superproject's
- **WHEN** `.github/workflows/ci.yml` is read
- **THEN** its one job `ci` uses `webcpporg/webcpp/.github/workflows/library.yml@main` with `library: idna`

#### Scenario: A newer pull request replaces an older run
- **WHEN** a second push reaches a pull request while its first run is going
- **THEN** the first run is cancelled (`cancel-in-progress` is true unless the ref is `refs/heads/main`)

### Requirement: Library: Licence notice and text rules
Every source file (`.hpp`, `.cpp`, `.py`, `.adoc`, `Jamfile`, `build.jam`, `.yml`) SHALL open with the notice "Copyright (c) 2026 WebCpp.org" and the Boost Software License 1.0 lines ("Distributed under the Boost Software License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)"), `//` for C++ and AsciiDoc, `#` for Python, Jam and YAML (after a `#!` line in a script). Every file MUST be in English and without the em dash. idna's code is under the Boost Software License alone; the generated files derived from Unicode's data, under the Unicode License v3, each name its notice in one comment line ("Unicode License v3: its notice is in the appendix "Unicode data" of idna's page"), and no separate licence file for Unicode is added.

#### Scenario: A file without the notice fails the lint
- **WHEN** a new `.hpp` is added without the licence notice
- **THEN** the superproject's lint fails, naming the file

#### Scenario: The generated tables name the notice
- **WHEN** `include/webcpp/idna/detail/tables.hpp` is read
- **THEN** its header says "Generated by tools/generate_tables.py from the Unicode 17.0.0 data files: do not edit." and names the appendix "Unicode data"

### Requirement: Errors: The nineteen errors
The library SHALL declare `enum class error : int` in `<webcpp/idna/errors.hpp>` with exactly these enumerators and values, one per family of rules rather than one per UTS 46 status code: `invalid_utf8` = 1 (input that is not well-formed UTF-8, RFC 3629), `disallowed` = 2 (UTS 46 V7, or V5: a U+002E inside a label), `not_idna2008` = 3 (a code point UTS 46 marks NV8 or XV8), `not_ldh` = 4 (UTS 46 U1: ASCII outside a-z, 0-9 and the hyphen), `not_nfc` = 5 (V1, RFC 5891 section 5.4), `hyphen_in_third_and_fourth` = 6 (V2), `hyphen_at_start_or_end` = 7 (V3), `ace_prefix` = 8 (V4: `xn--` while `check_hyphens` is off), `leading_combining_mark` = 9 (V6, RFC 5891 section 5.4), `contextj` = 10 (RFC 5892 A.1, A.2; IdnaTestV2 C1, C2), `contexto` = 11 (RFC 5892 A.3 to A.9), `bidi` = 12 (RFC 5893 section 2; IdnaTestV2 B1 to B6), `invalid_punycode` = 13 (RFC 3492 decoding, UTS 46 P4, an A-label not in its canonical form), `punycode_overflow` = 14 (RFC 3492 section 6.4, UTS 46 A3, or a label's Punycode past the conversions' bound), `empty_label` = 15 (A4_2, X4_2), `label_too_long` = 16 (A4_2: more than 63 octets), `domain_too_long` = 17 (A4_1: more than 253 octets), `output_too_small` = 18 (the caller's span cannot hold the result) and `storage_exhausted` = 19 (a label beyond the inline working storage, with no memory resource). Each enumerator MUST have a Doc Comment naming the steps of UTS 46, the status codes of IdnaTestV2 and the sections of the RFCs it reports.

#### Scenario: The values are 1 to 19 in order
- **WHEN** `errors_test` walks `invalid_utf8` through `storage_exhausted`
- **THEN** `static_cast<int>(e)` is 1, 2, 3, ..., 19 in that order

### Requirement: Errors: The values are fixed
The values of `idna::error` SHALL never change and never be reused for another meaning, so that a stored value keeps its meaning across versions; a new error takes the next value (20). `test/errors_test.cpp` pins every enumerator's value and message. The values 1 to 19 and the category's name and identity are part of the public contract.

#### Scenario: A stored value keeps its meaning
- **WHEN** an application stored `idna::error::contexto` as the integer 11 under 0.1.0 and reads it under a later version
- **THEN** `static_cast<idna::error>(11)` is still `contexto`

### Requirement: Errors: Messages
Each error SHALL have this human message, returned by the category's `message(int)`: `invalid_utf8` "invalid UTF-8", `disallowed` "disallowed code point", `not_idna2008` "code point disallowed by IDNA2008", `not_ldh` "ASCII code point other than a letter, a digit or a hyphen", `not_nfc` "label not in Normalization Form C", `hyphen_in_third_and_fourth` "hyphens in the third and fourth positions", `hyphen_at_start_or_end` "hyphen at the start or end of a label", `ace_prefix` "label starts with xn--", `leading_combining_mark` "label starts with a combining mark", `contextj` "joiner outside its allowed context", `contexto` "code point outside its allowed context", `bidi` "Bidi rule violated", `invalid_punycode` "invalid Punycode", `punycode_overflow` "Punycode overflow", `empty_label` "empty label", `label_too_long` "label longer than 63 octets", `domain_too_long` "domain name longer than 253 octets", `output_too_small` "output too small" and `storage_exhausted` "label beyond the working storage". A value `error` does not define (0 or 20 and above) MUST give "unknown webcpp.idna error".

#### Scenario: A message
- **WHEN** `idna::make_error_code(idna::error::invalid_punycode).message()` is called
- **THEN** it returns `invalid Punycode`

#### Scenario: An unknown value
- **WHEN** `idna::error_category().message(0)` and `message(20)` are called
- **THEN** both return `unknown webcpp.idna error`

### Requirement: Errors: The category
`idna::error_category()` SHALL return a `const boost::system::error_category&` that is a constant object of `detail::category` (a `constexpr` class deriving from `boost::system::error_category`) with the fixed name "webcpp.idna" and the fixed 64-bit identity `0x69646E6100000000` (the ASCII of "idna" followed by four zero bytes), so that idna keeps no mutable state of its own. Two categories with a non-zero identity compare by it alone. The category MUST override `message(int, char*, std::size_t)`, which copies the message into the caller's buffer cut to fit `size` bytes with its terminating zero, never writes past `size`, writes nothing when the buffer is null or `size` is 0, returns the whole message whatever the buffer, and is `noexcept`; `message(int)` returns a `std::string` and so may throw what `operator new` throws.

#### Scenario: The name and the identity
- **WHEN** `error_category().name()` is read and a test category of its own constructed with the identity `0x69646E6100000000` is compared with `error_category()`
- **THEN** the name is `webcpp.idna` and the two categories compare equal

#### Scenario: The message cut to a buffer
- **WHEN** `message(value, buffer, 8)` is called for `invalid_punycode` with a buffer filled with `x`
- **THEN** it returns the whole `invalid Punycode`, the buffer holds `invalid` and a zero, and the byte after the eighth is untouched

#### Scenario: A zero-sized or null buffer
- **WHEN** `message(value, buffer, 0)` or `message(value, nullptr, 8)` is called
- **THEN** it returns the whole message and writes nothing

#### Scenario: Which calls may throw
- **WHEN** the noexcept status of `error_category().message(0, nullptr, 0)`, of `error_category().message(0)` and of `make_error_code(error::invalid_utf8).message(char*, std::size_t)` is evaluated
- **THEN** the first and the third are `noexcept` and the second is not

### Requirement: Errors: Conversions and the result type
`idna::error` SHALL convert implicitly to `boost::system::error_code` (a specialization of `boost::system::is_error_code_enum<idna::error>` deriving from `std::true_type`), `idna::make_error_code(error)` MUST return the `error_code` with `static_cast<int>(e)` in the category `error_category()`, and an `error_code` MUST compare equal to an `idna::error` (`result.error() == idna::error::contexto`). `template <class T> using result = boost::system::result<T>;` is what every operation that can fail returns, holding a `T` (or nothing for `result<void>`) or the failure's `error_code`; `result<T> r = idna::error::invalid_utf8;` holds that error.

#### Scenario: A result holds an error
- **WHEN** `const idna::result<int> r = idna::error::invalid_utf8;` is evaluated
- **THEN** `r.has_error()` is true and `r.error() == idna::error::invalid_utf8`

#### Scenario: Each enumerator converts to its code
- **WHEN** `boost::system::error_code code = e;` is evaluated for every enumerator
- **THEN** `code.category() == idna::error_category()`, `code.message()` is the enumerator's message and `idna::make_error_code(e) == code`

### Requirement: Errors: The order in which errors are reported
A conversion or a check SHALL report the first error found, in this order: the labels' errors in the order of the labels, each label's by the order of its rules (see the validity rules); then `bidi` for the Bidi rule across the labels; then `domain_too_long`; and, in `to_ascii`, `output_too_small` last, so that a span too small for a name that is too long anyway reports the name. `to_unicode` goes on after an error and reports the first one it recorded, except that `output_too_small` replaces any other (the caller needs a larger span before anything else). A name with several faults therefore reports the earliest label's fault, never a later fault in a later label and never `bidi` ahead of a label error.

#### Scenario: Label order
- **WHEN** `to_ascii` is given `a_b.xn--zz` and, in the other order, `xn--zz.a_b`
- **THEN** the errors are `not_ldh` and `invalid_punycode` respectively

#### Scenario: A label error before the Bidi rule
- **WHEN** `to_ascii` is given `0a.\327\220.a_b`, a Bidi domain name whose last label has a fault
- **THEN** the error is `not_ldh`, never `bidi`

#### Scenario: Bidi before the length
- **WHEN** `to_ascii` is given `0a.\327\220.` followed by thirty nine-letter labels
- **THEN** the error is `bidi`

### Requirement: Options: The aggregate and its defaults
`<webcpp/idna/options.hpp>` SHALL declare `struct options`, an aggregate of eight `bool` members with these defaults, the strict profile: `check_hyphens = true` (UTS 46 CheckHyphens), `check_bidi = true` (CheckBidi), `check_joiners = true` (CheckJoiners), `use_std3_ascii_rules = true` (UseSTD3ASCIIRules), `verify_dns_length = true` (VerifyDnsLength), `transitional_processing = false` (Transitional_Processing), `ignore_invalid_punycode = false` (IgnoreInvalidPunycode) and `idna2008 = true` (idna's own: IDNA2008's validity on top of UTS 46's). Being an aggregate, a caller names only what changes with designated initializers, in declaration order: `idna::options{.idna2008 = false}` is UTS 46 alone.

#### Scenario: The defaults are strict
- **WHEN** a `constexpr idna::options defaults;` is read
- **THEN** `check_hyphens`, `check_bidi`, `check_joiners`, `use_std3_ascii_rules`, `verify_dns_length` and `idna2008` are true, and `transitional_processing` and `ignore_invalid_punycode` are false

#### Scenario: A caller names what changes
- **WHEN** `idna::options uts46_alone{.idna2008 = false}` is constructed
- **THEN** `idna2008` is false while `check_bidi` and `verify_dns_length` keep their defaults

### Requirement: Options: Constants
`options.hpp` SHALL declare `inline constexpr std::size_t max_label_length = 63` (the longest label of a domain name in its ASCII form, in octets, RFC 1034 section 3.1), `inline constexpr std::size_t max_domain_length = 253` (the longest domain name in its ASCII form without a trailing root label) and `inline constexpr std::string_view unicode_version = "17.0.0"` (the version of Unicode whose data the tables were generated from).

#### Scenario: The constants
- **WHEN** `options_test` reads them
- **THEN** `max_label_length` is 63, `max_domain_length` is 253 and `unicode_version` is `"17.0.0"`

### Requirement: Options: What each flag changes
Each member of `options` SHALL change the processing of every conversion and check exactly as follows. `check_hyphens` true refuses hyphens in both the third and the fourth positions (V2, `hyphen_in_third_and_fourth`) and at either end (V3, `hyphen_at_start_or_end`), and false refuses only a label that starts with `xn--` in any ASCII case (V4, `ace_prefix`). `check_bidi` true applies RFC 5893 section 2 to every label of a Bidi domain name. `check_joiners` true applies RFC 5892 A.1 and A.2 to U+200C and U+200D. `use_std3_ascii_rules` true refuses ASCII other than a-z, 0-9 and the hyphen (`not_ldh`). `verify_dns_length` true requires labels of 1 to 63 octets, a name of 1 to 253 and no empty root label. `transitional_processing` true maps the deviations (U+00DF to `ss`, U+03C2 to U+03C3, the joiners U+200C and U+200D removed) and U+1E9E to `ss`. `ignore_invalid_punycode` true keeps an `xn--` label that does not decode as it is, without an error. `idna2008` true refuses a code point UTS 46 marks NV8 or XV8 (`not_idna2008`) and applies the CONTEXTO rules of RFC 5892 A.3 to A.9.

#### Scenario: Hyphens off
- **WHEN** `ab--cd` is checked with `{.check_hyphens = false}`
- **THEN** it is valid, and `xn--xn---3ra` (an A-label decoding to a label that starts with `xn--`) is `ace_prefix`

#### Scenario: STD3 off
- **WHEN** `a_b` is checked with `{.use_std3_ascii_rules = false}` and then with `{.use_std3_ascii_rules = false, .idna2008 = false}`
- **THEN** the first is `not_idna2008` (U+005F carries the NV8 mark in the generated table) and the second is valid

#### Scenario: Joiners off
- **WHEN** `a` U+200C `b` is checked with the defaults and with `{.check_joiners = false}`
- **THEN** it is `contextj` and valid respectively

### Requirement: Punycode: Encoding into a caller's buffer
`punycode::encode(std::span<char> output, std::u32string_view input) noexcept` SHALL encode code points as RFC 3492 section 6.3 Punycode without the `xn--` prefix, and return a `[[nodiscard]] result<std::size_t>` holding the number of chars written. The basic code points (below U+0080) are copied as they are, case included, followed by a delimiter `-` when there is any; the digits that follow are lowercase. The encoding stops when `output` is full, so its time grows at most with the input's length times the size of `output`. It MUST return `invalid_punycode` for a surrogate or a value above U+10FFFF, found before anything is written whatever the size of `output`; `punycode_overflow` when the 32-bit arithmetic overflows (also when the input has 4,294,967,295 code points or more), found as the digits are written; and `output_too_small` when `output` cannot hold the result. After an error the contents of `output` are unspecified.

#### Scenario: RFC 3492 sample (L)
- **WHEN** `3年B組金八先生` is encoded into a 64-char array
- **THEN** the result is `3B-ww4c5e180e575a65lsy2b`

#### Scenario: All-basic input ends with the delimiter
- **WHEN** `abc` is encoded into a buffer of 4 chars, and into buffers of 0, 1, 2 and 3
- **THEN** the first yields 4 chars `abc-` and the others `output_too_small`

#### Scenario: A surrogate is found before anything is written
- **WHEN** `ab` followed by U+D800 is encoded into a 1-char buffer
- **THEN** the error is `invalid_punycode`, not `output_too_small`

### Requirement: Punycode: Decoding into a caller's buffer
`punycode::decode(std::span<char32_t> output, std::string_view input) noexcept` SHALL decode Punycode without its prefix (RFC 3492 section 6.2) into code points and return a `[[nodiscard]] result<std::size_t>` holding the number written. The code points before the last delimiter are copied as they are, case included; the digits after it are read in either case; a delimiter at the very start consumes nothing, so a leading `-` is then read as a digit and refused. At most `output.size()` code points are decoded, so the time grows at most with the input's length times the size of `output`. It MUST return `invalid_punycode` for a character that is not a digit, a non-basic code point (a byte above 0x7F) before the delimiter (found before anything is written), a truncated number, or a decoded value that is basic, a surrogate or above U+10FFFF; `punycode_overflow` for arithmetic past 32 bits (also for an input of 4,294,967,295 chars or more); and `output_too_small` when `output` cannot hold the result.

#### Scenario: RFC 3492 sample (L), decoded
- **WHEN** `3B-ww4c5e180e575a65lsy2b` is decoded into a 64-code-point array
- **THEN** 8 code points are written and they equal `3年B組金八先生`

#### Scenario: The failures of the Punycode example
- **WHEN** `-9uc`, `bcher-k` and `999999999999999999999` are decoded
- **THEN** the messages are `invalid Punycode`, `invalid Punycode` and `Punycode overflow`

#### Scenario: A value out of Unicode, then a surrogate
- **WHEN** `en32g`, `ib9b` and `zy0c` are decoded
- **THEN** each is `invalid_punycode` (above U+10FFFF, U+D800 and U+DFFF), while `hb9b` decodes to U+D7FF and `0y0c` to U+E000

#### Scenario: The first non-basic code point
- **WHEN** `a` is decoded
- **THEN** one code point, U+0080, is written

### Requirement: Punycode: The pmr forms
`punycode::encode(std::u32string_view input, std::pmr::memory_resource& resource = *std::pmr::get_default_resource())` SHALL return a `[[nodiscard]] result<std::pmr::string>` and `punycode::decode(std::string_view input, std::pmr::memory_resource& resource = *std::pmr::get_default_resource())` a `[[nodiscard]] result<std::pmr::u32string>`, each allocating from `resource` alone and returning the errors of the span form but `output_too_small`. The decoder sizes its output by the input's length (every code point consumes at least one character). Neither is `noexcept`: each may throw what its resource throws. Nothing bounds the input's length: the encoder passes over the input once for each distinct code point and the decoder inserts each code point among those before it, so their time grows with the square of it, and a program bounds the length of input it does not trust (the conversions bound a label at 256 code points).

#### Scenario: The pmr forms over a monotonic buffer
- **WHEN** `U"bücher"` is encoded and `bcher-kva` decoded with a `monotonic_buffer_resource` over a 512-byte buffer backed by `null_memory_resource()`
- **THEN** the results are `bcher-kva` and `bücher`, and `decode("-9uc", resource)` is `invalid_punycode`

#### Scenario: The pmr forms may throw
- **WHEN** `noexcept(punycode::encode(std::u32string_view()))` and `noexcept(punycode::decode(std::string_view()))` are evaluated, and likewise for the span forms
- **THEN** the pmr forms are not `noexcept` and the span forms are

### Requirement: Punycode: RFC 3492's samples and the cases they leave alike
The Punycode functions SHALL encode and decode all nineteen samples of RFC 3492 section 7.1, (A) to (S), both ways, with the encoder writing the digits in lowercase and the decoder reading the RFC's mixed-case annotation; the tests MUST also pin four inputs, checked against Python's punycode codec on 2026-10-09, that tell apart what the samples leave alike (a first delta whose division by damp, rather than by damp + 1, changes the bias; a delta that ends adapt's loop at exactly 455; and two whose thresholds differ under an initial bias of 70 or of 83): U+DA442 U+17C3 `w` is `w-kzjy4831q`, `b` U+36610 U+B1709 U+7A6F1 is `b-rp77aex91o9xjw`, U+24ED `p` is `p-4hp` and U+700C1 `nf` is `nf-gw73i`.

#### Scenario: Sample (D), Czech
- **WHEN** `Pročprostěnemluvíčesky` is encoded and `Proprostnemluvesky-uyb24dma41a` decoded
- **THEN** the encoding is `Proprostnemluvesky-uyb24dma41a` (the basic code points keep their case) and the decoding returns the original code points

#### Scenario: Sample (I), Russian, with an uppercase digit
- **WHEN** its code points are encoded and `b1abfaaepdrnnbgefbaDotcwatmq2g4l` is decoded
- **THEN** the encoding is `b1abfaaepdrnnbgefbadotcwatmq2g4l` (the digits are lowercase) and the decoding returns the original code points

#### Scenario: Sample (S)
- **WHEN** `-> $1.00 <-` is encoded
- **THEN** the result is `-> $1.00 <--`

### Requirement: Punycode: Case of digits and basic code points
The decoder SHALL read digits in either case and copy the basic code points as they are, and the encoder MUST NOT write a mixed-case annotation (its digits are lowercase, its basic code points as given).

#### Scenario: Digits in either case
- **WHEN** `bcher-kva`, `bcher-KVA` and `BCHER-kva` are decoded
- **THEN** the results are `bücher`, `bücher` and `BüCHER`

### Requirement: Punycode: Every overflow guard of RFC 3492 section 6.4
Every overflow of RFC 3492 section 6.4 SHALL be `punycode_overflow`, wherever it happens: in the decoder's `i` or `n`, in the encoder's delta or in its increments. The tests pin the boundary on both sides: `k0902716a` (2^32 - 1 as one number under the initial bias, so `n` overflows) and `k0902716b` (`i` overflows) are `punycode_overflow` while `ww902716a`, which reaches exactly 2^32 - 1, is `invalid_punycode` (beyond Unicode, not an overflow); an encoder input of 3,855 basic code points followed by U+10FFFF overflows its delta while 3,854 do not; 3,855 basic code points followed by U+10FF70 overflow in the increments; and 4,095 basic code points followed by U+10007F reach 2^32 - 1 exactly and encode to 4,095 `a` then `-k0902716a`, which decodes back.

#### Scenario: The decoder's overflow
- **WHEN** `k0902716a` and `k0902716b` are decoded
- **THEN** both are `punycode_overflow`

#### Scenario: The encoder's limit
- **WHEN** 3,854 `a` followed by U+10FFFF is encoded into a 4,096-char buffer, and 3,855 `a` followed by U+10FFFF
- **THEN** the first succeeds and the second is `punycode_overflow`

### Requirement: Punycode: An output of exactly the result's size suffices
The span forms SHALL succeed with an output of exactly the result's size and return `output_too_small` for every smaller size, writing nothing past the span: encoding `bücher` (9 chars `bcher-kva`) into every size from 0 to 8 is `output_too_small` with a sentinel byte after the span untouched and into 9 yields 9; decoding `bcher-kva` into 6 code points succeeds and into 0 to 5 is `output_too_small`; decoding `abc-` into 3 code points succeeds and into 2 is `output_too_small`.

#### Scenario: One short
- **WHEN** `bücher` is encoded into a span of 8 chars followed by a sentinel `#`
- **THEN** the error is `output_too_small` and the sentinel is still `#`

### Requirement: UTF-8: The strict decoder and encoder
`detail::utf8` SHALL decode and encode exactly RFC 3629 section 4's table of well-formed sequences: `decode(std::string_view text, std::size_t& i, char32_t& cp)` reads one code point at `i` and returns false, leaving `i` and `cp` as they were, for a lone continuation byte, an overlong form (C0 and C1 never lead; E0 80..9F, F0 80..8F), a surrogate (U+D800 to U+DFFF), a value above U+10FFFF (F4 90 and above), a lead byte F5 to FF, a truncated sequence, a lead followed by no continuation, and a bad continuation after a good one (`\342\202(`, `\342\202.`, `\360\237\230(`), so that the label separator is never swallowed into a code point; `is_valid(text)` is true exactly when every byte belongs to a well-formed sequence; `length(cp)` is 1, 2, 3 or 4 by the scalar value; `encode(cp, out)` writes the UTF-8 and returns its length. All are `constexpr` and `noexcept`. At the end of the text there is nothing to decode.

#### Scenario: The boundaries of every row
- **WHEN** `\0`, `\177`, `\302\200`, `\337\277`, `\340\240\200`, `\357\277\277`, `\360\220\200\200` and `\364\217\277\277` are decoded
- **THEN** they give U+0000, U+007F, U+0080, U+07FF, U+0800, U+FFFF, U+10000 and U+10FFFF, each consuming the whole text and encoding back to the same octets

#### Scenario: Refusals
- **WHEN** `\300\200`, `\355\240\200`, `\364\220\200\200`, `\377` and `\342\202.` are decoded
- **THEN** each is refused at its first byte with `i` still 0 and `cp` unchanged

#### Scenario: A string is valid only whole
- **WHEN** `is_valid` is given `b\303\274cher` and `b\303\274\303`
- **THEN** it is true for the first and false for the second

### Requirement: Validity: Checking a label as given
`check_label(std::string_view label, const options& opts = options()) noexcept` SHALL return a `[[nodiscard]] result<void>` that is empty for a label valid as it is given, a U-label, an A-label or an NR-LDH label, and otherwise the first error found. Nothing is mapped: a label with a capital letter (outside an A-label's `xn--` and Punycode) or not in NFC is refused. It MUST check, in this order: UTF-8 validity (`invalid_utf8`), then a U+002E inside the label (`disallowed`), then the label's rules in the order of the validity rules. The Bidi rule is applied to the label as to a domain of one label. An empty label is refused under `opts.verify_dns_length` (`empty_label`) and accepted without it. A label beyond the inline storage is `label_too_long` under `verify_dns_length` and `storage_exhausted` without it. Nothing allocates.

#### Scenario: Labels as the validity example prints them
- **WHEN** `example`, `xn--bcher-kva`, `l\302\267l`, `a\302\267l`, `ab--cd` and `Example` are checked with the defaults
- **THEN** the outcomes are valid, valid, valid, `code point outside its allowed context`, `hyphens in the third and fourth positions` and `disallowed code point`

#### Scenario: A full stop inside a label
- **WHEN** `a.b` is given to `check_label`
- **THEN** the error is `disallowed`

#### Scenario: Ill-formed UTF-8
- **WHEN** `\303` is given to `check_label`
- **THEN** the error is `invalid_utf8`

### Requirement: Validity: Checking a domain name as given
`check_domain(std::string_view domain, const options& opts = options()) noexcept` SHALL return a `[[nodiscard]] result<void>` for a whole name as given, splitting at U+002E alone, checking every label as `check_label` does, then the Bidi rule across the labels, then the lengths of the name. It MUST return the first error found: the labels' in order, then `bidi`, then `domain_too_long`; `storage_exhausted` for a label beyond the inline storage when `verify_dns_length` is off. The other full stops, U+3002, U+FF0E and U+FF61, separate labels only after UTS 46's mapping, so as given they are code points UTS 46 maps and are `disallowed`. Nothing allocates.

#### Scenario: The Bidi rule across labels
- **WHEN** `a.\327\220`, `0a.\327\220` and `0a.b` are checked as domains
- **THEN** the first is valid, the second is `bidi` (its first label starts with a digit and the name holds a right-to-left letter), and the third is valid (no right-to-left label, so no Bidi domain name)

#### Scenario: Other full stops as given
- **WHEN** `a\357\274\216b` and `a\343\200\202b` are checked as domains
- **THEN** both are `disallowed`

#### Scenario: An empty name and empty labels
- **WHEN** ``, `a..b` and `a.` are checked with the defaults, and `a.` and `a..b` with `{.verify_dns_length = false}`
- **THEN** the first three are `empty_label` and the last two are valid

### Requirement: Validity: The boolean forms
`is_valid_label(std::string_view, const options& = options()) noexcept` and `is_valid_domain(std::string_view, const options& = options()) noexcept` SHALL be `[[nodiscard]]` and return `check_label(...).has_value()` and `check_domain(...).has_value()` respectively.

#### Scenario: The booleans agree with the checks
- **WHEN** `validity_test` checks every sample with `check_domain` and `is_valid_domain`
- **THEN** the two agree on every one

### Requirement: Validity: The order of a label's rules
A label SHALL be checked in this order, and the first rule it breaks is its error: Normalization Form C (V1, RFC 5891 section 5.4: `not_nfc`); the hyphens under `check_hyphens` (V2 `hyphen_in_third_and_fourth`, V3 `hyphen_at_start_or_end`), or without it a label that starts with `xn--` (V4 `ace_prefix`); no leading combining mark (V6: `leading_combining_mark`); UTS 46's status (V7: every code point `valid`, or `deviation` in nontransitional processing; `disallowed`); STD3 ASCII under `use_std3_ascii_rules` (U1: `not_ldh`); CONTEXTJ under `check_joiners` (`contextj`); and, under `idna2008`, no NV8 or XV8 code point (`not_idna2008`) and then the CONTEXTO rules (`contexto`). `to_ascii` and `to_unicode` apply the same rules to each label after the mapping, with the NFC check only for a decoded A-label.

#### Scenario: Of two broken rules, the earlier
- **WHEN** `-e\314\201`, `\314\201a--b`, `\314\201ab-`, `\314\201A`, `A_`, `a_b\342\200\214`, `a\342\200\214\302\241` and `\302\241\302\267` are checked
- **THEN** the errors are `not_nfc`, `hyphen_in_third_and_fourth`, `hyphen_at_start_or_end`, `leading_combining_mark`, `disallowed`, `not_ldh`, `contextj` and `not_idna2008`

#### Scenario: A rule each
- **WHEN** `e\314\201`, `\314\201e`, `a_b` and `0\327\220` are checked
- **THEN** the errors are `not_nfc`, `leading_combining_mark`, `not_ldh` and `bidi`

### Requirement: Validity: A-labels
A label whose first four code points are `xn--`, its letters in either ASCII case, SHALL be an A-label: it MUST be ASCII, its Punycode MUST decode to a label that holds a code point beyond ASCII (UTS 46 P4), the decoded U-label is checked by the rules of the validity order (Normalization Form C included, and held to nontransitional processing whatever `transitional_processing` says) and encoded again, and an A-label that is not the encoding of its U-label, its letters compared in lowercase, is refused (RFC 5891 section 5.4). Each failure of the A-label itself (a code point beyond ASCII, Punycode that does not decode, P4, an empty decoding, a decoding to ASCII alone, or the round trip) is `invalid_punycode`, or `punycode_overflow` for a number past 32 bits. Under `ignore_invalid_punycode` a label whose Punycode does not decode is kept as it is and only its length is checked. A label whose first code points merely end in the bytes of `xn` (U+0278 U+026E followed by `--`) is no A-label.

#### Scenario: A-label case
- **WHEN** `xn--bcher-kva` and `XN--BCHER-KVA` are checked
- **THEN** both are valid

#### Scenario: A-labels that are not
- **WHEN** `xn--`, `xn--abc-`, `xn---9uc`, `xn--zz`, `xn--b\303\274cher` and `xn--bcher-kv\305\241` are checked
- **THEN** each is `invalid_punycode` (the last because U+0161 ends in the byte of `a` and must not be read as ASCII)

#### Scenario: Decoding to a disallowed code point
- **WHEN** `xn--ls8h` (U+1F4A9) is checked with the defaults and with `{.idna2008 = false}`
- **THEN** it is `not_idna2008` and valid respectively

#### Scenario: A decoded label held to NFC
- **WHEN** `xn--e-xbb` (an `e` and a combining acute accent) is checked
- **THEN** the error is `not_nfc`

#### Scenario: A label that only looks like an A-label
- **WHEN** U+0278 U+026E `--a` is checked with the defaults and with `{.check_hyphens = false}`
- **THEN** it is `hyphen_in_third_and_fourth` and valid respectively; and `xn----a-qvb3c` (its Punycode) behaves the same

### Requirement: Validity: Deviations in an A-label
A deviation such as U+00DF SHALL be valid as given in nontransitional processing alone, while an A-label's U-label is always held to nontransitional processing.

#### Scenario: Sharp s
- **WHEN** `fa\303\237` is checked with the defaults and with `{.transitional_processing = true}`, and `xn--fa-hia` with each
- **THEN** the first is valid and the second `disallowed`; `xn--fa-hia` is valid under both

### Requirement: Validity: CONTEXTJ
With `check_joiners`, U+200C ZERO WIDTH NON-JOINER SHALL be allowed only after a virama (Canonical_Combining_Class 9) or between a code point of Joining_Type L or D and one of R or D with only T between (RFC 5892 A.1), and U+200D ZERO WIDTH JOINER only after a virama (A.2), at every occurrence; the joining types come from the generated Joining_Type table, never from a list of scripts (NKo, Mandaic and Phags-pa are among those a hand-written list misses). `detail::context::check_joiners(std::u32string_view)` implements it and `is_contextj` is true for U+200C and U+200D only.

#### Scenario: Allowed non-joiners
- **WHEN** `check_joiners` is given Devanagari KA, VIRAMA, ZWNJ, SSA; ARABIC BEH, ZWNJ, BEH; BEH, ZWNJ, ALEF; PHAGS-PA U+A872, ZWNJ, BEH; BEH, FATHATAN, ZWNJ, FATHATAN, BEH; NKo U+07CA, ZWNJ, U+07CA; and Mandaic U+0841, ZWNJ, U+0841
- **THEN** all are accepted

#### Scenario: Refused non-joiners
- **WHEN** ALEF, ZWNJ, BEH; BEH, ZWNJ, U+A872; BEH, HAMZA, ZWNJ, BEH; BEH, ZWNJ, HAMZA, BEH; ZWNJ, BEH; BEH, ZWNJ; `a`, ZWNJ, `b`; TATWEEL, ZWNJ, BEH; and BEH, ZWNJ, TATWEEL are given
- **THEN** all are refused (R before, L after, a non-joining U neighbour, a joiner at an edge, and join-causing C is neither L nor D)

#### Scenario: Every occurrence is checked
- **WHEN** KA, VIRAMA, ZWNJ, SSA, `x`, ZWNJ, `y` is given
- **THEN** it is refused: the first occurrence is allowed and the second is not

#### Scenario: The joiner
- **WHEN** KA, VIRAMA, ZWJ, SSA; BEH, ZWJ, BEH; and ZWJ alone are given
- **THEN** the first is accepted and the others refused

### Requirement: Validity: CONTEXTO
With `idna2008`, the CONTEXTO code points SHALL satisfy RFC 5892 A.3 to A.9 at every occurrence: U+00B7 MIDDLE DOT between two `l` (A.3); U+0375 GREEK KERAIA before a Greek code point (A.4); U+05F3 GERESH and U+05F4 GERSHAYIM after a Hebrew code point (A.5, A.6); U+30FB KATAKANA MIDDLE DOT in a label that holds a Hiragana, Katakana or Han code point (A.7); ARABIC-INDIC DIGITS (U+0660 to U+0669) never in a label with EXTENDED ARABIC-INDIC DIGITS (U+06F0 to U+06F9) (A.8, A.9). `detail::context::check_others(std::u32string_view)` implements them in time linear in the label's length: a label without a CONTEXTO code point returns after one pass, and one with them takes a second pass for its facts (`label_facts`: kana-or-Han, Arabic-Indic digit, extended Arabic-Indic digit), never a pass per occurrence. `is_contexto` is true for U+00B7, U+0375, U+05F3, U+05F4, U+30FB and the two digit ranges (tested at their ends U+0660, U+0669, U+06F0 and U+06F9) and false for the code points just outside the ranges, U+065F, U+066A, U+06EF and U+06FA, and for `l`.

#### Scenario: Middle dot
- **WHEN** `l·l`, `a·l`, `·l`, `l·a` and `l·` are given to `check_others`
- **THEN** only the first is accepted

#### Scenario: Keraia, geresh and gershayim
- **WHEN** alpha KERAIA beta, alpha KERAIA `S`, alpha KERAIA, BET GERESH, BET GERSHAYIM, GERESH BET and `a` GERSHAYIM are given
- **THEN** the first, fourth and fifth are accepted and the others refused

#### Scenario: Katakana middle dot
- **WHEN** U+30FB U+3041, U+30A2 U+30FB, U+4E00 U+30FB, `def` U+30FB `abc`, alpha U+30FB alef and U+30FB alone are given
- **THEN** the first three are accepted and the rest refused

#### Scenario: Digits
- **WHEN** U+0660 U+0661, U+06F0 U+06F1, BEH U+0660, U+0660 U+06F0 and U+06F9 U+0669 are given
- **THEN** the first three are accepted and the last two refused

#### Scenario: A long label in linear time
- **WHEN** labels of 4,096 U+30FB followed by U+3041, 4,096 U+0660 followed by U+0661 and 4,096 U+0660 followed by U+06F0 are given
- **THEN** they are accepted, accepted and refused respectively, and the timing test holds the time under 1,000 times that of an equally long label without a CONTEXTO code point

### Requirement: Validity: The Bidi rule
With `check_bidi`, `detail::bidi::check(std::u32string_view label)` SHALL apply the six rules of RFC 5893 section 2 (IdnaTestV2's B1 to B6) to one label and return a `label_bidi` with `rtl` (the label holds an R, AL or AN code point, which makes its domain a Bidi domain name) and `satisfied`. Rule 1: the first code point is L, R or AL. In an RTL label (first code point R or AL) rules 2 to 4 apply: only R, AL, AN, EN, ES, CS, ET, ON, BN and NSM are allowed; it ends with R, AL, EN or AN followed by NSMs; and EN and AN do not both occur. In an LTR label (first code point L) rules 5 and 6 apply: only L, EN, ES, CS, ET, ON, BN and NSM; it ends with L or EN followed by NSMs. An empty label is neither RTL nor in breach of a rule. A domain is refused with `bidi` when one label holds an R, AL or AN code point and any label (itself included) does not satisfy the rules.

#### Scenario: Right-to-left labels
- **WHEN** alef-bet, alef-lam, alef plus NSM, alef `1`, alef plus ARABIC-INDIC ZERO are checked
- **THEN** each is RTL and satisfied; while alef `a`, alef `-`, alef `!`, alef `0` ARABIC-INDIC ZERO alef and alef space alef are RTL and not satisfied

#### Scenario: Left-to-right labels
- **WHEN** `abc`, `a0`, `a` plus NSM, `a-b`, `a,b`, `a$b`, `a!b` are checked
- **THEN** each is not RTL and satisfied; while `a-` and `a b` are not satisfied, and `a` alef, `a` AL, `a` AN are RTL and not satisfied

#### Scenario: The first code point
- **WHEN** `0a`, `0` alef, ARABIC-INDIC ZERO, `-a`, `!a` and NSM `a` are checked
- **THEN** none is satisfied

### Requirement: Validity: IDNA2008's property is derived from UTS 46's table
A code point SHALL be PVALID, CONTEXTJ or CONTEXTO in IDNA2008 exactly when its UTS 46 status is `valid`, or `deviation`, and it bears no NV8 or XV8 mark; one that does bear the mark is `not_idna2008`. U+0000 is `valid` with the mark, U+00A1 INVERTED EXCLAMATION MARK is `valid` with the mark, and U+002E is the separator no label holds. The generator MUST check this derivation against IANA's `idna-tables-12.0.0` on every run (see the generator's checks); no IANA table is shipped.

#### Scenario: NV8 code points
- **WHEN** `detail::uts46_of(0x0000)` and `uts46_of(0x00A1)` are read, and `check_domain("xn--7a")` runs
- **THEN** both entries are `valid` with `not_idna2008` set, and the domain is `not_idna2008` with the defaults and valid with `{.idna2008 = false}`

#### Scenario: The strings UTS 46 alone accepts
- **WHEN** `validity_test` checks the fifteen invalid `idn-hostname` strings of the JSON Schema suite that UTS 46 alone accepts, as U-labels, and thirteen A-labels of its hostname tests (plus `0a.xn--4db`)
- **THEN** all are refused: a Hangul name holding U+302E, U+0640 U+07FA, and U+3031 to U+3035 with U+302E, U+302F and U+303B are `not_idna2008`; a misplaced middle dot (`a\302\267l`, `\302\267l`, `l\302\267a`, `l\302\267`), keraia (alpha, U+0375, `s`; alpha, U+0375), geresh and gershayim first, and katakana middle dot (`def`, U+30FB, `abc`; U+30FB alone) are `contexto`; `0a.` followed by alef is `bidi`; `xn--7a` is `not_idna2008`; among the A-labels, `xn--07jt112bpxg.xn--9t4b11yi5a`, `xn--chb89f` and `xn--07jceefgh4c` are `not_idna2008`, `xn--al-0ea`, `xn--l-fda`, `xn--la-0ea`, `xn--l-gda`, `xn--s-jib3p`, `xn--wva3j`, `xn--5db1e`, `xn--5db3e`, `xn--defabc-k64e` and `xn--vek` are `contexto`, and `0a.xn--4db` is `bidi`

#### Scenario: Their valid neighbours
- **WHEN** `l\302\267l`, alpha KERAIA beta, BET GERESH, katakana middle dot with a Hiragana letter, `a.alef`, `0a.b`, `xn--bcher-kva.example` and `b\303\274cher.example` are checked
- **THEN** all are valid

### Requirement: Validity: Lengths
With `verify_dns_length`, `check_label` and `check_domain` SHALL measure a label by its A-label: a label of 63 octets is valid and 64 is `label_too_long`; 55 `a` and a U+00FC make an A-label of 63 octets and 56 make one of 64; a name of 253 octets is valid and 254 is `domain_too_long`, with separators counted before each label; an empty label is `empty_label` (the root label of `a.` included). A U-label's length is its A-label's, and an A-label given as such is measured as given.

#### Scenario: 63 and 64
- **WHEN** `check_label` is given 63 `a`, 64 `a`, `xn--` plus 55 `a` plus `-8yf`, and `xn--` plus 56 `a` plus `-t2f`
- **THEN** the first and third are valid and the second and fourth are `label_too_long`

#### Scenario: 253 and 254
- **WHEN** a name of three 63-`a` labels and one 61-`a` label (253 octets) is checked, and the same with an `a` appended
- **THEN** the first is valid and the second is `domain_too_long`; the same holds when the three 63-octet labels are A-labels given as such

### Requirement: Validity: Storage
A label beyond the working storage of 256 code points SHALL be refused as soon as it outgrows it, while it is gathered, before its rules: `label_too_long` under `verify_dns_length` (no such label can pass the DNS lengths), `storage_exhausted` without it. The storage holds 256 code points as given and refuses the 257th; it also holds the label after canonical decomposition for the NFC check's scratch copy, so 128 U+00E9 followed by U+0301 (129 code points as given, 257 decomposed) are `label_too_long` under the lengths and `storage_exhausted` without. An error recorded in an earlier label is kept: `a_b` followed by a 300-`a` label is `not_ldh`.

#### Scenario: The edge
- **WHEN** 256 `a` and 257 `a` are checked with `{.verify_dns_length = false}`
- **THEN** the first is valid and the second is `storage_exhausted`

#### Scenario: Both errors
- **WHEN** 300 `a` is given to `check_label` with the defaults and with `{.verify_dns_length = false}`
- **THEN** the errors are `label_too_long` and `storage_exhausted`

#### Scenario: The first error survives
- **WHEN** `a_b.` followed by 300 `a` is checked as a domain with the defaults and with `{.verify_dns_length = false}`
- **THEN** both are `not_ldh`

### Requirement: Validity: Linear time in the number of labels
`check_domain` SHALL do a bounded amount of work per label: a name of 65,536 one-letter labels is checked in less than 20 times the time of a name as long made of labels of 255 letters (the fastest of three runs each, `verify_dns_length` off), a ratio near 1 on every lane where a pass over the name per label would take it past 100.

#### Scenario: Many labels
- **WHEN** `many_labels_in_linear_time` runs
- **THEN** the measured time is below `20 * baseline`

### Requirement: Validity: NUL and the C0 controls
NUL and the other C0 controls inside a `std::string_view` SHALL be ASCII outside the STD3 rules and NV8 in UTS 46's table: `a\0b` (3 chars) is `not_ldh` with the defaults, `not_idna2008` with `{.use_std3_ascii_rules = false}`, and valid only with `{.use_std3_ascii_rules = false, .idna2008 = false}`.

#### Scenario: A NUL
- **WHEN** `check_label(std::string_view("a\0b", 3))` runs with each of the three option sets
- **THEN** the results are `not_ldh`, `not_idna2008` and valid

### Requirement: Conversion: The processing
`to_ascii` and `to_unicode` SHALL implement UTS 46 section 4: each code point is looked up in the mapping table (a `valid` code point stays; a `mapped` one is replaced by its mapping, `A` by `a`, U+FB01 by `fi`, U+3002 by `.`; an `ignored` one such as U+00AD is removed; a `deviation` stays unless the processing is transitional; a `disallowed` one stays, to be refused when its label is checked), the result is normalized to NFC, split at U+002E, and each label that starts with `xn--` is decoded from Punycode and checked as the U-label it holds while any other is checked as it is. It MUST work one label at a time in a working storage of one label's code points, so a name of any length needs the same small storage.

#### Scenario: Mapping makes a name valid
- **WHEN** `to_ascii` is given `B\303\234CHER.Example`
- **THEN** the result is `xn--bcher-kva.example`

#### Scenario: Quick start
- **WHEN** `idna_quick_start` converts `B\303\274cher.Example` and `a\302\267b.example`, and `to_unicode` converts `xn--bcher-kva.example`
- **THEN** it prints `Bücher.Example -> xn--bcher-kva.example`, `a·b.example -> error: code point outside its allowed context` and `bücher.example`

### Requirement: Conversion: ToASCII into a caller's buffer
`to_ascii(std::span<char> output, std::string_view domain, const options& opts = options()) noexcept` SHALL write the ASCII form of `domain` (UTS 46 section 4.2) into `output` and return a `[[nodiscard]] result<std::size_t>` holding the number of chars written: each label that is not ASCII as `xn--` and its Punycode, the others as they are, joined by `.`. A name that fails any check has no ASCII form: the result is the first error (see the order of errors), `output_too_small` last. With `opts.verify_dns_length` the result never exceeds `max_domain_length`, so a `std::array<char, idna::max_domain_length>` always holds it and the error is never `output_too_small` in a buffer of that size. After an error the contents of `output` are unspecified. Nothing allocates. `storage_exhausted` is returned for a label beyond the inline storage when `verify_dns_length` is off.

#### Scenario: The longest name that fits
- **WHEN** twenty-five nine-letter labels and `.abc` (253 octets) are converted into a `std::array<char, 253>`, and the same name with `d` appended
- **THEN** the first returns 253 chars equal to the input and the second is `domain_too_long`

#### Scenario: The length comes before the buffer
- **WHEN** `nine_letter_labels(30) + ".x"` and twenty labels `b\303\274cher` (279 octets as ASCII) are converted into a 253-char array
- **THEN** both are `domain_too_long`, never `output_too_small`, while `b\303\274cher.example` into a 5-char array is `output_too_small`

#### Scenario: The defaults are strict
- **WHEN** `a_b.example`, `example.` and `\342\231\245.example` are converted
- **THEN** the errors are `not_ldh`, `empty_label` and `not_idna2008`

### Requirement: Conversion: ToASCII into a pmr string
`to_ascii(std::string_view domain, const options& opts = options(), std::pmr::memory_resource& resource = *std::pmr::get_default_resource())` SHALL return the same ASCII form as a `[[nodiscard]] result<std::pmr::string>` from `resource`, which also holds the working storage of a label beyond the inline capacity. It MUST report the same first error as a span form large enough to hold the result, for every name, and is never `output_too_small`: under `verify_dns_length` a label beyond the inline storage is refused as the span form refuses it (`label_too_long`), and without it a label of any length is converted. A label of more than 256 code points that is not ASCII, or an A-label whose Punycode decodes to more, is `punycode_overflow`. It may throw only what `resource` throws when it cannot allocate.

#### Scenario: Both forms report the same error
- **WHEN** `a_b` plus 300 `a`, `a_b.` plus 300 `a`, and the A-label of a hyphen, 128 U+00E9 and U+0301 are converted by both forms with the defaults
- **THEN** both forms give `label_too_long`, `not_ldh` and `label_too_long` respectively

#### Scenario: Without the lengths
- **WHEN** a 300-`a` label is converted with `{.verify_dns_length = false}` by the span form (into a 400-char buffer) and by the pmr form
- **THEN** the span form returns `storage_exhausted` and the pmr form returns the 300 octets; with the defaults the span form returns `label_too_long`

#### Scenario: The allocation example
- **WHEN** `idna_allocation` runs
- **THEN** it prints `xn--bcher-kva.example` twice (span form, then a pmr form over a 16,384-byte monotonic buffer on the stack backed by `null_memory_resource()`), `label beyond the working storage` and `300 octets`

### Requirement: Conversion: ToUnicode into a caller's buffer
`to_unicode(std::span<char> output, std::string_view domain, const options& opts = options()) noexcept` SHALL write the Unicode form (UTS 46 section 4.3, in UTF-8) into `output` and return a `[[nodiscard]] unicode_result` of `std::size_t size` (the chars written) and `boost::system::error_code error` (empty when the processing succeeded, else the first error, or `output_too_small` when the buffer could not hold the whole result, which takes the place of any other). As UTS 46 asks, the conversion goes on after an error: each label is written as the processing leaves it, mapped and normalized, an A-label decoded when its Punycode decodes and kept as it is otherwise, with no U+FFFD inserted. When the span is full the writing stops at a code point boundary. Ill-formed UTF-8 writes nothing and is `invalid_utf8`. A label beyond the inline storage stops the writing there with `storage_exhausted`, after any earlier label's error. Nothing allocates.

#### Scenario: Conversion goes on after an error
- **WHEN** `A..xn--bcher-kva` is converted
- **THEN** the output is `a..b\303\274cher` with the error `empty_label`

#### Scenario: A code point boundary
- **WHEN** `b\303\274cher` is converted into a 2-char buffer, and `a_b` into a 2-char buffer
- **THEN** the first writes 1 char with `output_too_small` and the second writes `a_` with `output_too_small` (replacing `not_ldh`)

#### Scenario: Ill-formed input
- **WHEN** `a\303` is converted
- **THEN** 0 chars are written and the error is `invalid_utf8`

#### Scenario: An A-label that decodes to nothing
- **WHEN** `xn--` is converted
- **THEN** 0 chars are written and the error is `invalid_punycode`

#### Scenario: Output that ends at the long label
- **WHEN** `a_b.` plus 300 `a` is converted
- **THEN** the output is `a_b` and the error is `not_ldh`

### Requirement: Conversion: ToUnicode into a pmr string
`to_unicode(std::string_view domain, const options& opts = options(), std::pmr::memory_resource& resource = *std::pmr::get_default_resource())` SHALL return a `[[nodiscard]] unicode_string` of `std::pmr::string value` (the Unicode form in UTF-8) and `boost::system::error_code error` (the first error recorded). It is never `output_too_small`, checks no label's or name's maximum length, and converts a label of any length (an A-label within Punycode's bound of 256 decoded code points); an A-label whose Punycode decodes to more than 256 code points is kept as it is with `punycode_overflow` whatever `ignore_invalid_punycode` says. It may throw only what `resource` throws.

#### Scenario: A long label
- **WHEN** a 300-`a` label is converted with the defaults by the pmr form
- **THEN** the label is written whole with no error, where the span form stops with `storage_exhausted`

### Requirement: Conversion: The four separators
The conversions SHALL split labels at U+002E and, after the mapping, at U+3002, U+FF0E and U+FF61: each alone or at an edge of a name leaves an empty label (`empty_label`) and each between two labels splits them, so `a` U+3002 `b` is `a.b`.

#### Scenario: Each separator
- **WHEN** `to_ascii` is given each of the four alone, before `example`, after `example`, and between `a` and `b`
- **THEN** the first three give `empty_label` and the last gives `a.b`

### Requirement: Conversion: The root label
A final empty label (the DNS root, as in `example.`) SHALL be an error in `to_ascii` under `verify_dns_length` (`empty_label`) and no error in `to_unicode` (UTS 46 X4_2 exempts only a final empty label that follows another), while `.example`, `a..b` and `a..` are `empty_label` in the Unicode form too; without `verify_dns_length` both forms pass the root label through, and `to_ascii` of `` and `a..b` gives them as they are.

#### Scenario: The Unicode form keeps the root
- **WHEN** `to_unicode` is given `example.` and `.example`
- **THEN** the first writes `example.` with no error and the second has `empty_label`

#### Scenario: Without the lengths
- **WHEN** `to_ascii` is given `example.` with `{.verify_dns_length = false}`
- **THEN** the result is `example.`

### Requirement: Conversion: Transitional processing
With `transitional_processing`, the deviations SHALL be mapped (U+00DF to `ss`, U+03C2 to U+03C3, U+200C and U+200D removed) and U+1E9E to `ss`; the third column of IdnaTestV2 (toAsciiT) is checked with it on. Without it, a deviation stays and the label is checked as it is.

#### Scenario: Capital sharp s
- **WHEN** `\341\272\236` is converted with `{.transitional_processing = true}`
- **THEN** the result is `ss`

#### Scenario: Line 107 of IdnaTestV2
- **WHEN** `fa\303\237.de` is converted nontransitionally and transitionally
- **THEN** the results are `xn--fa-hia.de` and `fass.de`

### Requirement: Conversion: Invalid Punycode
An `xn--` label whose Punycode does not decode SHALL be `invalid_punycode` (or `punycode_overflow`), and with `ignore_invalid_punycode` it MUST be kept as it is, without an error, held to the DNS lengths in the ASCII form alone (the Unicode form has none), and unchecked by the Bidi rule. An `xn--` label with an undecodable Punycode that is also longer than 63 octets is then `label_too_long` in the ASCII form.

#### Scenario: Kept
- **WHEN** `xn--zz.example` is converted with the defaults and with `{.ignore_invalid_punycode = true}`
- **THEN** the first is `invalid_punycode` and the second returns `xn--zz.example`

#### Scenario: Kept but too long
- **WHEN** `xn--_` followed by seventy `9` is converted to ASCII with `{.ignore_invalid_punycode = true}`
- **THEN** the error is `label_too_long`

### Requirement: Conversion: Control characters
`to_unicode` of a name with NUL SHALL write it as it is, with `not_ldh`; `to_ascii` follows the validity rules for NUL (see the NUL requirement), passing it through only with both `use_std3_ascii_rules` and `idna2008` off.

#### Scenario: NUL in the Unicode form
- **WHEN** `a\0b` (3 chars) is given to `to_unicode`
- **THEN** the 3 chars are written and the error is `not_ldh`

### Requirement: Limits: The working storage
The conversions and the checks SHALL hold one label at a time in a working storage of 256 code points inside the call (`detail::max_label_code_points = 256`; about 1.3 KiB with the label's ASCII form, and a second one while a label is checked for NFC the slow way). Every label the DNS lengths accept fits, because an A-label decodes to at most 59 code points and a code point decomposes to at most 4 (U+1FAF). A label beyond 256 code points once decomposed is too long for the DNS and meets `label_too_long` under `verify_dns_length` (both forms of `to_ascii` and the validators), `storage_exhausted` without it (the span form of `to_ascii` and the validators) or the label's storage taken from the memory resource (the pmr form of `to_ascii`). `to_unicode` checks no label's maximum length and meets such a label with any options: its span form stops there with `storage_exhausted` and its pmr form converts it. The storage fills while a label is gathered, before any of the label's checks, so U+FFFF followed by 300 `a` is `label_too_long`, not `disallowed`.

#### Scenario: Decomposition past the storage
- **WHEN** 255 `a` and U+00E9 (257 code points decomposed, 256 once composed) are converted with `{.verify_dns_length = false}` by the span form and by the pmr form
- **THEN** the span form is `storage_exhausted` and the pmr form converts it

#### Scenario: Reported while gathered
- **WHEN** U+FFFF followed by 300 `a` is converted to ASCII with the defaults
- **THEN** the error is `label_too_long`

### Requirement: Limits: Punycode's bound
The conversions SHALL bound a label's Punycode at 256 code points (`detail::max_punycode_code_points`, the working storage's): the encoder's input in `to_ascii` and the decoder's output in `to_ascii` and `to_unicode`, past which a label is `punycode_overflow` whatever `ignore_invalid_punycode` says (a limit of the implementation is no invalid Punycode), and in the Unicode form the A-label is kept as it is. `to_unicode` encodes nothing, so a label past the bound that is not an A-label is written as it is. Only a pmr form meets the bound (`to_ascii` without the DNS lengths, and `to_unicode`, with any options, for an A-label), since a span form refuses such a label as beyond its storage first. The decoder MUST stop at the bound before reading the rest of the label: the Punycode of 300 distinct ideographs followed by `!` is `punycode_overflow`, where a decoder that read it whole would find the character first, and the same character after 200 ideographs is `invalid_punycode`. At the bound the worst label costs about 4 times one as long that Punycode handles in linear time (measured at `-O2`).

#### Scenario: At the bound and beyond
- **WHEN** a label of 256 distinct ideographs and one of 257 are converted to ASCII without the DNS lengths, and the A-label of each is converted to ASCII and to Unicode
- **THEN** the 256 encode (and decode back), the 257 are `punycode_overflow` in each, and the Unicode form of the 257's A-label is the A-label itself with `punycode_overflow`

#### Scenario: A long label that is not an A-label
- **WHEN** 257 and 300 distinct ideographs are given to the pmr `to_unicode`
- **THEN** each is written as it is with no error

#### Scenario: The encoder does no work beyond the bound
- **WHEN** a label of 16,384 distinct ideographs is refused by the pmr `to_ascii` without the lengths
- **THEN** it is refused in less than 10 times the time of a label of as many copies of one ideograph

### Requirement: Limits: The stack
idna SHALL NOT recurse, and its deepest call, a check of a label or a conversion of an A-label whose U-label holds more than 32 combining marks in a row, which the slow NFC check sorts by counting, was measured at about 11 KiB of stack (the conversions and checks of an ordinary name such as `xn--bcher-kva.example` at about 8 KiB, a program's start and `printf` included) in a debug build (`-O0 -g`) of wasi-sdk 34 for wasm32-wasip2 and wasm32-wasip3 under wasmtime 47, against the 64 KiB a WebAssembly program gets by default. These are measured figures with no margin promised.

#### Scenario: A 64 KiB stack is enough
- **WHEN** an ordinary name is converted in a wasm32-wasip2 program with the default stack
- **THEN** it completes, using about 8 KiB

### Requirement: NFC: Normalization Form C within a label's storage
`detail::nfc` SHALL implement Normalization Form C (Unicode 17.0.0 section 3.11, UAX 15) as canonical decomposition as code points are appended (`append_decomposed`, which returns false when the label's storage is full), then canonical ordering and canonical composition in place (`compose_label`), each step doing a bounded amount of work per code point however long the label and its runs of non-starters: a decomposition is at most 4 code points long, a composition looks back at one starter, and canonical ordering sorts a run of at most 32 non-starters (`max_insertion_run`) by insertion, a longer run up to an inline label's length by a stable counting sort through a buffer on the stack (never inlined, so it is on the stack only while sorted), and a longer run in a growing label by a counting sort through a buffer from the label's memory resource. Hangul syllables are decomposed and composed by the arithmetic of Unicode section 3.12 and take no table. `is_nfc(text, scratch)` answers `verdict::yes`, `verdict::no` or `verdict::exhausted`: NFC_Quick_Check answers Yes or No at once (a non-starter out of order is No), and for Maybe the text is normalized into the empty `scratch` label and compared.

#### Scenario: Every line of NormalizationTest.txt
- **WHEN** `nfc_test` normalizes the three columns (source, NFC, NFD) of each of the 20,034 lines
- **THEN** each gives the NFC column, `is_nfc` of the NFC column is `yes`, and `is_nfc` of the source is `yes` exactly when it equals the NFC column

#### Scenario: Hangul
- **WHEN** U+AC01 is decomposed and composed
- **THEN** it decomposes to U+1100 U+1161 U+11A8 and composes back to U+AC01

#### Scenario: The quick check's Maybe
- **WHEN** `e` followed by U+0301 and U+00E9 followed by U+0301 are given to `is_nfc`
- **THEN** the first is `no` (it composes) and the second `yes`; and 255 U+00E9 followed by U+0301 (256 code points, 511 decomposed) given with an inline scratch is `exhausted`

#### Scenario: Canonical ordering in linear time
- **WHEN** a run of 10,000 non-starters is normalized
- **THEN** it takes less than 20 times the time of a text as long without one, which an insertion sort exceeds by far

### Requirement: Storage: The inline and the growing label
`detail::inline_label` SHALL hold a label's code points in fixed storage that never allocates (`push_back` returns false when the 256 are full; `shrink`, `clear`, `span`, `view`, `reset(size)` for a decoder to write into, and `keep_ascii()` copying an all-ASCII label to chars of its own with its letters lowercased), and `detail::growing_label` MUST hold them inline and then in a `std::pmr::vector` of the memory resource it was given once they outgrow it (`push_back` never fails), with the same interface and `resource()`. `fresh_like` gives an empty label of the same kind for a scratch copy.

#### Scenario: A full label
- **WHEN** 256 `a` are pushed onto an `inline_label`, then one more
- **THEN** the 257th `push_back` returns false and `append_decomposed` of U+00E9 returns false

#### Scenario: A growing label
- **WHEN** 257 `a` are pushed onto a `growing_label` over a 4,096-byte monotonic buffer
- **THEN** all succeed and `view().size()` is 257; `shrink(3)` leaves `aaa`; `clear()` leaves it empty; `reset(300)` and `reset(4)` give spans of 300 and 4

#### Scenario: An A-label's chars
- **WHEN** `XN--Bcher-KVA` is pushed and `keep_ascii()` called
- **THEN** it returns `xn--bcher-kva`

### Requirement: Driver: The label driver
`detail::process(domain, opts, mode, form, label, sink)` SHALL be the one streaming driver of UTS 46 section 4 that `uts46.hpp` and `validity.hpp` share: `mode::map` maps and normalizes each code point first (`to_ascii`, `to_unicode`) and `mode::as_given` takes each label as it is, split at U+002E only (`check_label`, `check_domain`); `form::ascii` hands the ASCII form (with the DNS lengths) to the sink and stops at the first error, `form::unicode` hands the Unicode form and records every error and goes on. A sink takes pieces: `span_sink` writes each piece whole while it fits and marks itself cut after one did not, `string_sink` appends to a `std::pmr::string`, and `discard_sink` keeps nothing (the validators). The driver over an inline label into a span or into nothing is `noexcept`; over a growing label or a string sink it may throw what the resource throws. A separator is handed to the sink with the label's first piece, so a label that stops the driver leaves none behind.

#### Scenario: The driver does not throw for the span forms
- **WHEN** `noexcept(process(...))` is evaluated for (`inline_label`, `span_sink`) and (`inline_label`, `discard_sink`), and for (`growing_label`, `span_sink`) and (`inline_label`, `string_sink`)
- **THEN** the first two are `noexcept` and the last two are not

#### Scenario: As given, the ASCII form of an A-label is the label
- **WHEN** `xn--bcher-kva.XN--BCHER-KVA.b\303\274cher.a` is processed `as_given` into the ASCII form
- **THEN** the sink holds `xn--bcher-kva.xn--bcher-kva.xn--bcher-kva.a`

### Requirement: Driver: Empty labels
An empty label SHALL be `empty_label` under `verify_dns_length` for both forms, but for a final empty label that follows another in the Unicode form (UTS 46 X4_2); `xn--` decodes to an empty label and is `invalid_punycode` with nothing read of it (IdnaTestV2.txt line 550) in both forms, as given too; without `verify_dns_length` an empty label is accepted and passed through. A read past an empty label MUST trap in the tests, which build with the standard libraries' own checks on.

#### Scenario: The empty labels of the driver
- **WHEN** ``, `.a`, `a..b` and `a.` are processed `map` in each form with the defaults, and `a..` in the Unicode form
- **THEN** the first three are `empty_label` in both forms, `a.` is `empty_label` in the ASCII form and `a.` with no error in the Unicode form, and `a..` is `empty_label`

#### Scenario: A decoded label that is empty
- **WHEN** `a.xn--.b` is processed in the Unicode form
- **THEN** the error is `invalid_punycode` and the sink holds `a..b`

### Requirement: Driver: A label beyond the storage after an earlier error
A label beyond the storage, gathered or in the NFC check's copy, SHALL stop the driver but keep an error recorded before it, and the Unicode form writes nothing after the last label it ended. A growing label goes on instead: the same A-label (`xn--9ca`, 127 `a`, `8520e`: 128 U+00E9 and a combining acute) is valid and written whole by the pmr driver after `a.` (260 octets: `a.`, 128 U+00E9 and U+0301 at two octets each).

#### Scenario: The NFC copy stops the driver
- **WHEN** `a_b.` plus that A-label plus `.c` is processed in the Unicode form with an inline label, and the A-label alone in the Unicode and ASCII forms
- **THEN** the first has `not_ldh` and writes `a_b`, the second is `storage_exhausted`, and the third `label_too_long`

### Requirement: Tables: Generated constexpr data
`include/webcpp/idna/detail/tables.hpp` SHALL be generated, never edited by hand: `constexpr` arrays of constant data in `webcpp::idna::detail::tables` (`uts46_runs` of `std::uint64_t`, `uts46_mappings`, `bidi_classes`, `joining_types`, `scripts`, `combining_classes`, `marks`, `nfc_quick_checks`, `decomposition_keys`, `decomposition_slices`, `decomposition_data`, `composition_keys`, `composition_values`), the enumerations `status` (`valid`, `mapped`, `deviation`, `ignored`, `disallowed`), `bidi_class` (`other`, `l`, `r`, `al`, `an`, `en`, `es`, `cs`, `et`, `on`, `bn`, `nsm`), `joining_type` (`other`, `l`, `d`, `r`, `t`), `script` (`other`, `greek`, `hebrew`, `hiragana`, `katakana`, `han`) and `quick_check` (`yes`, `no`, `maybe`), and `unicode_version = "17.0.0"`. No table is built at run time, none is compressed, nothing is allocated and nothing is shared mutable state, so any number of threads may call idna at once. Each property is a sorted list of runs looked up by binary search, and the packing of each array is stated in the Doc Comment above it. The tables hold 147,324 bytes of data, and a program links only those it reads.

#### Scenario: The tables are data
- **WHEN** `properties_test` evaluates `detail::uts46_of(U'A').mapping == U"a"`, `bidi_class_of(0x05D0) == tables::bidi_class::r`, `combining_class_of(0x0300) == 230`, `decomposition_of(0x00E9).size() == 2` and `composition_of(U'e', 0x0301) == 0xE9` as `static_assert`s
- **THEN** all hold at compile time

### Requirement: Tables: The lookups
`detail/properties.hpp` SHALL expose these `constexpr noexcept` lookups, each a binary search over sorted runs with no state and no allocation, over code points of at most U+10FFFF (as the UTF-8 and Punycode decoders guarantee; a packed key keeps 24 bits of a code point in a run list and 21 in a composition pair): `uts46_of(cp)` (a `uts46_entry` of `status`, `not_idna2008` and `mapping`), `bidi_class_of`, `joining_type_of`, `script_of`, `combining_class_of`, `is_mark`, `nfc_quick_check_of`, `decomposition_of` (empty for no decomposition and for Hangul syllables, which are algorithmic) and `composition_of(first, second)` (0 when none).

#### Scenario: Spot values
- **WHEN** `properties_test` reads U+00DF, U+00AD, U+3002, U+FDFA, U+E000 and U+10FFFF
- **THEN** U+00DF is `deviation` mapping to `ss`, U+00AD is `ignored`, U+3002 maps to `.`, U+FDFA has the longest mapping of the table (18 code points), U+E000 and U+10FFFF are `disallowed`

#### Scenario: Other tables
- **WHEN** it reads U+05FF (unassigned in the Hebrew block), U+0627, U+0628, U+0300, U+094D, U+0375, U+30FB, U+4E00, U+0340, U+0301, U+1FAF and U+0958+U+093C
- **THEN** U+05FF is Bidi R (an `@missing` line); U+0627 joins R and U+0628 D; U+0300 is joining T with class 230; U+094D has combining class 9; U+0375 is Greek; U+30FB is script `other` (Common); U+4E00 is Han; U+0340 is quick-check No and U+0301 Maybe; U+1FAF decomposes to four code points; and the exclusion U+0958 composes from U+0915 U+093C to nothing (0)

### Requirement: Tables: The generator
`tools/generate_tables.py` (Python 3.9 or newer, standard library only) SHALL write the tables and the conformance vectors from the inputs `tools/unicode.json` pins: usage `generate_tables.py [--cache DIR] [TABLES IDNA_VECTORS NFC_VECTORS]`, exit 0 when every output was written, 1 naming a fault, 2 on a usage error. A file missing from the cache directory (default `bin/unicode/<version>/` of the superproject) is downloaded into it; every file is hashed, and a digest that differs from the pinned one stops the generator, naming the file (or the URL it was downloaded from, which then leaves nothing in the cache) and both digests, before anything is written. The three outputs are `include/webcpp/idna/detail/tables.hpp`, `test/generated/idna_test_v2.ipp` (IdnaTestV2.txt as rows) and `test/generated/nfc_test.ipp` (the first three columns of NormalizationTest.txt as rows). Two runs MUST write the same bytes (no date, sorted data, `\n` line endings), and nothing is written unless every check passed. A property value it does not know (a script a new Unicode version adds) stops it by name; the value joins its list once it is known not to matter to IDNA.

#### Scenario: A moved or altered file
- **WHEN** a pinned file's digest differs
- **THEN** the generator exits 1 naming the file and both digests, and writes nothing

#### Scenario: A new property value
- **WHEN** a Unicode file holds a script value that neither `SCRIPTS` nor `SCRIPT_OTHERS` lists
- **THEN** the generator stops naming it

### Requirement: Tables: What the generator checks
Before it writes, the generator SHALL check what idna assumes of the data and stop naming the assumption that fails: every status of the mapping table is one of the five it knows; IANA's IDNA2008 property of every code point IANA assigns one agrees with the derivation from UTS 46's table, U+002E FULL STOP aside; the CONTEXTJ and CONTEXTO code points are those of RFC 5892 appendix A; no full canonical decomposition is longer than 4 code points; no Hangul syllable has a decomposition in `UnicodeData.txt`; and every property value a file holds is one it knows. It MUST check the test files as strictly: every line of IdnaTestV2.txt has its seven fields, its status fields are blank or lists of the status codes the file's header names, and its one escape is `\uXXXX` standing for one code point (a pair of escaped surrogates, which could mean one code point or two, stops it); every line of NormalizationTest.txt has its five columns of code points, none a surrogate; and `test/known_failures.json` is for the pinned version and each entry names a line of IdnaTestV2.txt by number and by its source as the file writes it, once, with a reason.

#### Scenario: A longer decomposition
- **WHEN** a Unicode version adds a canonical decomposition of 5 code points
- **THEN** the generator stops, because the label storage's measurement (4) no longer holds

### Requirement: Tables: Pinned inputs
`tools/unicode.json` SHALL pin `version` 17.0.0 and ten files, each by `path`, `url` (under `https://www.unicode.org/Public/17.0.0/`, or IANA's `https://www.iana.org/assignments/idna-tables-12.0.0/idna-tables-properties.csv`) and `sha256`: `idna/IdnaMappingTable.txt`, `idna/IdnaTestV2.txt`, `ucd/UnicodeData.txt`, `ucd/extracted/DerivedBidiClass.txt`, `ucd/extracted/DerivedJoiningType.txt`, `ucd/extracted/DerivedCombiningClass.txt`, `ucd/DerivedNormalizationProps.txt`, `ucd/Scripts.txt`, `ucd/NormalizationTest.txt` and `iana/idna-tables-properties.csv`, the last read only by the generator's checks.

#### Scenario: The pin of the mapping table
- **WHEN** `tools/unicode.json` is read
- **THEN** `idna/IdnaMappingTable.txt` carries the sha256 `87f05505dc026fdb2bff16132bdc68a8014675836882a9a2b1844540ad3be382`

### Requirement: Tables: The own lane that checks generated files
`test/Jamfile` SHALL declare `webcpp.generated tables : ../tools/generate_tables.py : : ../include/webcpp/idna/detail/tables.hpp generated/idna_test_v2.ipp generated/nfc_test.ipp ;` and `webcpp.lane tables : generated-tables ;`: the own lane `tables` runs the generator into the build directory and compares each output with the committed file, byte for byte, naming each that differs; `b2 libs/idna/test//update-generated` writes them in place and is the one writer of the generated files in a build. The ordinary lanes never run the generator, which may need the network; the CI runs the lane on its own.

#### Scenario: A stale table
- **WHEN** the generator's output differs from the committed `tables.hpp`
- **THEN** `b2 -a libs/idna/test//tables` fails naming `tables.hpp`

### Requirement: Tables: Moving to another version of Unicode
A move to another version of Unicode SHALL be one change of its own that updates, in this order: `tools/unicode.json` (the version and the url and sha256 of each Unicode file; IANA's tables stay) and `test/known_failures.json` (its `unicode`, and no entry); `tools/generate_tables.py` run by `update-generated`; `options.hpp`'s `unicode_version` (which `detail/properties.hpp` refuses to compile while it differs from the tables'), the version `test/options_test.cpp` and `test/properties_test.cpp` pin, and the counts and line numbers `test/vectors_test.cpp` pins; the attributes `:unicode-version:` and `:uts46-revision:` of `doc/idna.adoc`; the text that names the version (`meta/libraries.json`'s description, `README.md`'s first paragraph and `AGENTS.md`'s section on Unicode); every lane, with the known failures brought up to date; and the changelog. The move to Unicode 18.0.0 (UTS 46 revision 36) is not made.

#### Scenario: The page lists the files
- **WHEN** the section "Moving to another version of Unicode" of `doc/unicode.adoc` is read
- **THEN** it lists those seven steps

### Requirement: Tables: Compile cost
Only `<webcpp/idna/detail/properties.hpp>` SHALL include the tables, and of the public headers only `uts46.hpp` and `validity.hpp` reach it: a translation unit that includes `<webcpp/idna.hpp>` and calls `to_ascii`, compiled at `-O2`, was measured at about 0.7 s of processor time and an object of 188 KB natively with Apple Clang 21, and about 0.8 s and 197 KB for wasm32-wasip2 with wasi-sdk 34, against 0.14 s and 0.07 s, and under 1 KB, for the same program without idna (the least of fifteen compilations, user and system time, on an Apple M5 Max shared with other work). The tests that compile the generated vectors, large files of data, MUST be built with `/bigobj` on MSVC, and the vectors' strings are `std::string_view` literals (`"..."sv`) so that they stay within every compiler's constant-evaluation steps.

#### Scenario: The vectors on MSVC
- **WHEN** `test/Jamfile` declares `vectors`, `nfc`, `validity` and `uts46`
- **THEN** each carries `<toolset>msvc:<cxxflags>/bigobj`

### Requirement: Tables: The Unicode data notice
The generated files (`tables.hpp`, `test/generated/idna_test_v2.ipp`, `test/generated/nfc_test.ipp`) derive from the Unicode Character Database and the files of UTS 46, version 17.0.0, which Unicode, Inc. publishes under the Unicode License v3 (Copyright 1991-2026 Unicode, Inc.). The page's appendix "Unicode data" (anchor `idna-unicode-data`) SHALL hold the notice whole and say that idna's code is under the Boost Software License 1.0 alone, and `README.md` MUST say the same in its section "Unicode data".

#### Scenario: The appendix
- **WHEN** `doc/unicode-data.adoc` is read
- **THEN** it holds "UNICODE LICENSE V3" and "Copyright © 1991-2026 Unicode, Inc." and the sentence that idna's code is under the Boost Software License 1.0 alone

### Requirement: Guarantees: No span form and no validator allocates
No span form of a conversion (`to_ascii`, `to_unicode`, `punycode::encode`, `punycode::decode` into a span) and no validator (`check_label`, `check_domain`, `is_valid_label`, `is_valid_domain`) SHALL allocate, whatever its input; a pmr form allocates from its memory resource alone. This is guarantee 1 (anchor `idna-invariant-1`).

#### Scenario: Counted allocations
- **WHEN** `allocation_test` replaces the global `operator new` for its program alone and runs every span form and validator over fourteen hostile inputs (a 300-`a` label, 500 one-letter labels, ill-formed UTF-8, `xn--zz.xn--.xn---9uc.xn--ls8h`, right-to-left labels, joiners, decomposing sequences, middle dots, full-width forms, hyphen faults, a 70-`a` label with U+00FC, and the empty string) with the strict profile, UTS 46 alone and no DNS lengths, plus the Punycode encoder over four code-point inputs
- **THEN** the count of calls of `operator new` does not change and some results succeed

#### Scenario: The pmr forms allocate through their resource
- **WHEN** the same inputs go through `to_ascii` (without the lengths), `to_unicode` and `punycode::decode`, and the encoder inputs through `punycode::encode`, with a counting resource over `new_delete_resource()`
- **THEN** the calls of the global `operator new` equal the resource's allocations, and there is at least one

### Requirement: Guarantees: Nothing throws but a memory resource
No conversion, validator or Punycode function SHALL throw but the allocation of the memory resource a pmr form was given. The category's `message(int)` returns a `std::string` and so may throw what the global `operator new` throws; `error_code::message(char*, std::size_t)`, which calls the category's `message(int, char*, std::size_t)`, copies into a caller's buffer and throws nothing. This is guarantee 2 (`idna-invariant-2`). The span forms and the validators MUST be `noexcept`, the pmr forms not, and a test's `static_assert`s state it.

#### Scenario: noexcept is stated
- **WHEN** `static_assert(noexcept(idna::to_ascii(std::declval<std::span<char>>(), std::string_view())))`, the same for `to_unicode`, `check_label`, `check_domain`, `is_valid_label` and `is_valid_domain`, and `static_assert(!noexcept(idna::to_ascii(std::string_view())))` and for the pmr `to_unicode` are compiled
- **THEN** they hold

### Requirement: Guarantees: Determinism across targets
The same input and options SHALL give the same output and the same error on every target (guarantee 3, `idna-invariant-3`); every test builds with libc++'s extensive hardening (`_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE`) and libstdc++'s assertions (`_GLIBCXX_ASSERTIONS`, each ignoring the other's macro) so that a read past the end of a view traps instead of reading a neighbour's memory.

#### Scenario: One set of vectors
- **WHEN** `uts46` runs on native, wasip2, wasip3 and Emscripten
- **THEN** each passes the same 6,391 lines

### Requirement: Guarantees: The length bound of to_ascii
With `verify_dns_length`, `to_ascii` SHALL return at most `max_domain_length` (253) chars and report a name whose ASCII form is longer as `domain_too_long`, unless an earlier error comes first in the order of the errors, and never as `output_too_small` in a buffer of that size (guarantee 4, `idna-invariant-4`). After an error, the buffer of the span form of `to_ascii`, as of a Punycode span form, holds nothing to read; the span form of `to_unicode` goes on after an error, as UTS 46 asks, and the `size` chars of its `unicode_result` are the name as the processing left it.

#### Scenario: Nothing to read after an error
- **WHEN** `to_ascii` fails
- **THEN** the contents of `output` are unspecified

### Requirement: Guarantees: The tables are the generator's output
The tables SHALL be exactly what `tools/generate_tables.py` writes from the pinned Unicode 17.0.0 files (guarantee 5, `idna-invariant-5`), which the own lane `tables` checks. The guarantees keep their numbers and anchors forever; code, tests and comments cite them.

#### Scenario: The anchors
- **WHEN** `doc/guarantees.adoc` is read
- **THEN** it holds the anchors `idna-invariant-1` to `idna-invariant-5`

### Requirement: Tests: The unit tests and how they are registered
`test/Jamfile` SHALL declare, with webcpp's rules, `webcpp.headers-alone idna : ../include` (every header compiles alone, 15 of them), `webcpp.compile` for `config`, `config_developer_defined` and `config_all_off` (all from `config_test.cpp`), `webcpp.run` for fourteen lightweight_test programs `errors`, `options`, `utf8`, `punycode`, `properties`, `vectors`, `known_failures`, `nfc`, `context`, `bidi`, `process`, `validity`, `uts46` and `allocation` (each `<name>_test.cpp`), `webcpp.run-fail` for `known_failures_unexpected_pass` and `known_failures_unexpected_failure`, and the own lane `tables`. Every test builds with `_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE` and `_GLIBCXX_ASSERTIONS`. `test/.clang-tidy` and `example/.clang-tidy` inherit the parent configuration and switch off `bugprone-exception-escape.CheckMain`, so that a test's `main` lets what a failing case throws end the process.

#### Scenario: The suite
- **WHEN** `b2 libs/idna/test` runs from the root of a webcpp checkout
- **THEN** the fourteen programs run and pass, the two run-fail builds fail as expected, each header compiles alone and `config.hpp` compiles three times

### Requirement: Tests: Conformance to IdnaTestV2
`uts46_test` SHALL run every line of IdnaTestV2.txt (6,391 rows, from line 106 to line 6508, toUnicode, toAsciiN and toAsciiT) through the span and the pmr forms of `to_ascii` and `to_unicode`, first with `{.idna2008 = false}` as the file assumes (toAsciiT with `transitional_processing = true`), then the strict profile over the 3,084 lines whose toUnicode field holds no NV8, XV8 or CONTEXTO code point (the file's `strict` flag), requiring the two forms to agree. A source that is not well-formed UTF-8 (the lines with a lone surrogate, which no UTF-8 string holds) MUST write nothing and be `invalid_utf8`, as the file allows. idna passes every line of it.

#### Scenario: Line 107
- **WHEN** the row `fa\303\237.de` of line 107 (toUnicode equal to the source, toAsciiN `xn--fa-hia.de`, toAsciiT `fass.de`) is run
- **THEN** both forms agree with every column

#### Scenario: A lone surrogate
- **WHEN** line 548, `a`, the three bytes `\355\244\200` (U+D900) and `z`, is run
- **THEN** `to_unicode` writes nothing and the error is `invalid_utf8`

### Requirement: Tests: The generated vectors
`test/idna_vectors.hpp` and `test/nfc_vectors.hpp` SHALL declare `webcpp::test::idna_vector` (fields `line`, `source`, `to_unicode`, `to_unicode_error`, `to_ascii_n`, `to_ascii_n_error`, `to_ascii_t`, `to_ascii_t_error`, `strict`, `known_failure`) and `nfc_vector` (`source`, `nfc`, `nfd`) and include the generated `.ipp` files, so no test reads a file on any target. `vectors_test` MUST pin that there are 20,034 NFC rows and 6,391 IdnaTestV2 rows from line 106 to 6508; that blank fields are resolved as the file's header says (line 107: toUnicode is the source, toAsciiT `fass.de`; line 109: toAsciiT is toAsciiN; line 153, `a` ZWNJ `b` with `[C1]`: an explicit `[]` is no error though toAsciiNStatus inherits `[C1]` and transitional processing drops the ZWNJ; line 209, an A-label of 64 octets: toAsciiTStatus is toAsciiNStatus, `[A4_2]`; line 543, `$`: toAsciiN is toUnicode; line 550, `xn--`: `""` is the empty string); that line 548 holds the three bytes of the lone surrogate; and that 3,084 rows are strict.

#### Scenario: Blank fields
- **WHEN** `vectors_test` reads line 550
- **THEN** the source is `xn--`, toUnicode is empty and toAsciiN is empty

### Requirement: Tests: Known failures never lie
`test/known_failures.json` SHALL list the IdnaTestV2 lines expected to fail as `{"unicode": "17.0.0", "failures": []}` (empty: idna passes every line), each entry, if there were one, by number and source with a reason that names both versions of Unicode, the only kind it may hold being a failure from a difference between two versions of Unicode, and the generator writes each reason into its line's row. A conformance test holds each verdict against its row through `webcpp::test::as_listed(line, known_failure, passed)`, which is true when `passed == known_failure.empty()` and otherwise reports a failed check naming the line, `passes, and test/known_failures.json lists it: <reason>` or `fails, and test/known_failures.json does not list it`. A listed line must fail under UTS 46 alone and the strict pass leaves it out.

#### Scenario: Both directions
- **WHEN** `known_failures_test` is built as is, with `WEBCPP_TEST_IDNA_UNEXPECTED_PASS` and with `WEBCPP_TEST_IDNA_UNEXPECTED_FAILURE`
- **THEN** the first passes (`as_listed(107, "", true)` and `as_listed(107, "a reason", false)` hold), and the two others are declared with `webcpp.run-fail` and fail

### Requirement: Tests: Timing tests are relative
A test that guards a complexity bound SHALL compare the time of the guarded input with a baseline of the same size taken in the same run, the fastest of three runs each, never with an absolute time: NFC's run of 10,000 non-starters under 20 times its baseline; `check_others` under 1,000 times (the baseline takes microseconds, so a loaded machine has taken the ratio to 41, and the pass per occurrence it guards against would be past 30,000); `check_domain` of 65,536 labels under 20 times; the pmr `to_ascii` of 16,384 distinct ideographs under 10 times. The decoder's bound is pinned without any timing.

#### Scenario: No absolute clock
- **WHEN** the timing tests are read
- **THEN** each compares `measured` with `slower * baseline` or `N * baseline`, and none with a constant number of nanoseconds

### Requirement: Docs: The documentation page
The library SHALL have one documentation page, `doc/idna.adoc`, titled "idna: internationalized domain names for C++", built by `webcpp.doc idna : idna.adoc ;` and `webcpp.reference idna ;` in `doc/Jamfile` (`b2 libs/idna/doc`, into `doc/html/index.html`, published at https://webcpporg.github.io/libs/idna/), with the attributes `:unicode-version: 17.0.0` and `:uts46-revision: 35` that every mention reads. It MUST include, in this order, the sections Introduction, Punycode, Converting names, Validity, Errors, Allocation, Limits, Unicode and the tables, Guarantees and Testing, then the Reference (every name of `webcpp::idna` and `webcpp::idna::punycode` from the Doc Comments of the headers, with `show-enum-constants: true` in `doc/mrdocs.yml` so that each enumerator has a section of its own, its value and its whole Doc Comment), and the appendices "Unicode data" and "Changelog". Every public symbol MUST have a Doc Comment, and each `@see "<title>"` names a top-level section; renaming a section renames its `@see` lines in the same commit, and doc-check fails otherwise. Every program the page shows is compiled and run natively and on wasm32-wasip2, wasm32-wasip3 and Emscripten, and printed the output shown.

#### Scenario: The index lists the library
- **WHEN** the superproject's `b2 doc` runs with `libs/idna` present
- **THEN** the index lists idna from its `meta/libraries.json`, linked to its page

#### Scenario: A missing Doc Comment
- **WHEN** a public function has no Doc Comment
- **THEN** the strict MrDocs reference fails the documentation build

### Requirement: Docs: Counts come from the data
`doc/counts.py <library-directory>` SHALL print `n-idna-test-v2=<rows>` (the lines of `test/generated/idna_test_v2.ipp` that start with `    idna_vector{`) and `n-nfc-vectors=<rows>` (those of `nfc_test.ipp` that start with `    nfc_vector{`), which `tools/doc/counts.py` of the superproject gives the page as attributes beside the counts of every library; a count that finds nothing fails rather than put a zero on the page, a fault goes to the standard error and the standard output holds only counts. Exit 0 printing the counts, 1 naming a fault, 2 on a usage error.

#### Scenario: The counts
- **WHEN** `doc/counts.py` runs on the library at `e2452e5`
- **THEN** it prints `n-idna-test-v2=6391` and `n-nfc-vectors=20034`

### Requirement: Docs: An A-label in prose
An A-label in the page's prose SHALL be written as literal monospace with the plus-passthrough, `` `+xn--bcher-kva.example+` ``, because Asciidoctor replaces `--` between two word characters with an em dash, inside backticks too, and no check of the build sees it in the rendered page; `xn--` alone followed by a backtick is safe. The address of the Unicode directory on the page MUST be text, not a link inside code.

#### Scenario: The page has no em dash
- **WHEN** the rendered page is searched for the em dash
- **THEN** none is found

### Requirement: Docs: Examples
The library SHALL ship four examples in `example/`, declared in `example/Jamfile` by `webcpp.example`, each compiled and run on every target and compared with the `.expected` file beside it, and the page includes them by tag: `idna_quick_start.cpp` (tag `program`), `idna_punycode.cpp` (tags `encode`, `decode`, `errors`), `idna_validity.cpp` (tags `labels`, `domains`, `uts46_alone`) and `idna_allocation.cpp` (tags `span`, `pmr`, `long_label`).

#### Scenario: The Punycode example
- **WHEN** `idna_punycode` runs
- **THEN** it prints `3B-ww4c5e180e575a65lsy2b`, `8 code points, the same: yes`, `-9uc -> invalid Punycode`, `bcher-k -> invalid Punycode` and `999999999999999999999 -> Punycode overflow`

#### Scenario: The validity example
- **WHEN** `idna_validity` runs
- **THEN** it prints `example: valid`, `xn--bcher-kva: valid`, `l·l: valid`, `a·l: code point outside its allowed context`, `ab--cd: hyphens in the third and fourth positions`, `Example: disallowed code point`, `a.א: valid`, `0a.א: invalid`, `0a.b: valid` and `a·l under UTS 46 alone: valid`

### Requirement: Docs: Changelog
`doc/changelog.adoc` SHALL be the page's last appendix (anchor `idna-changelog`), with a section "Unreleased" between the tags `unreleased` whose lines are, in Boost's release-notes form: "Added the library: internationalized domain names over Unicode 17.0.0 tables.", "Added Punycode encoding and decoding, into a caller's buffer or a pmr string.", "Added UTS 46 ToASCII and ToUnicode, passing every line of IdnaTestV2.", "Added IDNA2008 validity of labels and domains, with its contextual and Bidi rules.", "Added errors as values with a fixed category, and options for every check." and "Added the `WEBCPP_IDNA_NO_EXCEPTIONS` configuration macro, which `WEBCPP_NO_EXCEPTIONS` also defines." Every change adds a line.

#### Scenario: The unreleased section
- **WHEN** `doc/changelog.adoc` is read
- **THEN** it holds those six lines between `// tag::unreleased[]` and `// end::unreleased[]`

### Requirement: Docs: README
`README.md` SHALL describe idna as a header-only library of internationalized domain names for C++20 over Unicode 17.0.0 tables, its default profile (UTS 46's mapping followed by IDNA2008's validity, the strict profile JSON Schema's `idn-hostname` asks for), the guarantees in one paragraph (errors as values, no allocation in the span forms, pmr for the string forms), the requirements (Boost's headers and webcpp's configuration, config, nothing to build or link), the quick start program with its output (included from `example/idna_quick_start.cpp` and `.expected`), how to build and test (`b2 libs/idna/test libs/idna/example`, and the toolsets `clang-wasip2`, `clang-wasip3` and `emscripten`), where the documentation is, the Unicode data notice and the licence.

#### Scenario: The README's output
- **WHEN** the README's quick start is run
- **THEN** it prints the three lines of `idna_quick_start.expected`
