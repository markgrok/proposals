---
title: "IDNA for JSON Schema's idn formats"
tags: [webcpp, json-schema, idna, spike]
status: active
created: 2026-10-09
---

# IDNA for JSON Schema's idn formats

A measurement spike for `webcpp::json_schema`. Question: can the formats
`idn-hostname` and `idn-email` use an existing IDNA library, or must webcpp
generate its own Unicode tables? The owner's rule: if an existing library
fails more than 10% of the relevant suite tests, look elsewhere; otherwise use
it. Background: `~/.buzz/RESEARCH/JSON_SCHEMA_FOR_WEBCPP.md` section 4.1 (the
IDN tests in detail, ICU measured at 8 of 92) and the method of
`~/.buzz/RESEARCH/JSON_SCHEMA_ENGINE_SPIKE.md`.

Everything ran on 2026-10-09, macOS on Apple silicon, and is throwaway under
`~/.claude/jobs/fcf3a902/tmp/spike-idna/` (called `$S` below);
`$S/run.sh` rebuilds and re-measures all of it (log in `$S/run.log`, reports
in `$S/report_*.md`).

| Input | Version |
| --- | --- |
| JSON-Schema-Test-Suite | commit `5b0ee16`, `~/.claude/jobs/fcf3a902/tmp/jsonschema-research/JSON-Schema-Test-Suite` |
| ada-url/idna | https://github.com/ada-url/idna commit `92ea246969cdf25e792f1e2dee30a653468cd418` (2026-10-06), 28 commits after tag `v0.6.0` (`acdd2e2`, 2026-07-10); CMake version 0.6.0; Unicode/IDNA 17.0.0 |
| ada-url/ada | https://github.com/ada-url/ada commit `4eee4c2` (2026-10-05), `v4.0.0-42` |
| upa-url/idna | https://github.com/upa-url/idna commit `1e47bfa` (2026-10-04), Unicode 18.0.0 |
| libidn2 | 2.3.8 from Homebrew |
| ICU | icu4c 78.3 from Homebrew |
| UCD | Unicode 17.0.0: `IdnaMappingTable.txt`, `DerivedJoiningType.txt`, `DerivedBidiClass.txt`, `Scripts.txt`, `DerivedCombiningClass.txt`, `UnicodeData.txt`, `DerivedNormalizationProps.txt` from https://www.unicode.org/Public/17.0.0/; IANA `idna-tables-12.0.0` (the newest IANA registry) |
| Native compiler | Apple clang, libc++, `-std=c++20 -O2` |
| WASI | wasi-sdk 34 (clang 23.1.0) at `~/dev/webcpp/.local/wasi-sdk`, wasmtime 47.0.3; wasip2 `-fno-exceptions`, wasip3 `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false -lunwind` run with `-W exceptions=y`, and wasip3 `-fno-exceptions` |

## Summary

| Library | idn-hostname failed (per draft, all four identical) | idn-email failed | idn-hostname + idn-email, all drafts | hostname (A-labels) | Against the 10% rule |
| --- | --- | --- | --- | --- | --- |
| **ada-url/idna `92ea246`** + UTS 46 flags in the harness | **15/92 = 16.3%** | 0/18 (v1 0/19) | 60/441 = **13.6%** | 13/58 = 22.4% | **over** |
| ada-url/ada's bundled copy (amalgamation of 2026-07-12) | 17/92 = 18.5% | 0/18 | 68/441 = 15.4% | 13/58 | over |
| upa-url/idna (UTS 46, every flag on) | 14/92 = 15.2% | 0/18 | 56/441 = 12.7% | 13/58 | over |
| libidn2 2.3.8 `idn2_lookup_u8` (TR46 non-transitional, STD3) | 13/92 = 14.1% | 0/18 | 52/441 = 11.8% | 11/58 | over |
| libidn2 2.3.8 lookup + `idn2_register_u8` per A-label | 2/92 = 2.2% | 0/18 | 8/441 = 1.8% | 0/58 | under, but LGPL-3.0+/GPL-2.0+, C, libunistring |
| ICU 78.3 `uidna_openUTS46` + CONTEXTJ/CONTEXTO/Bidi, with the harness's DNS rules | 4/92 = 4.3% | 0/18 | 16/441 = 3.6% | 3/58 | under, but ICU |
| ada + a suite-fitted thin layer (no tables, about 60 lines) | 1/92 = 1.1% | 0/18 | 4/441 = 0.9% | 0/58 | under, but not correct beyond the suite |

ada's IDNA fails 16.3% of `idn-hostname` in every draft, so the rule sends
us elsewhere. Every UTS 46 library (ada, upa, ICU without a layer, libidn2's
lookup) fails the same way, because the suite asks for IDNA2008 (RFC 5891,
5892, 5893) and UTS 46 is not IDNA2008: UTS 46 marks the RFC 5892
DISALLOWED exceptions and symbols such as U+00A1 as valid (`NV8`/`XV8` in
`IdnaMappingTable.txt`), and does not define CONTEXTO at all. The one library
under 10% without ICU is libidn2 with its registration check, which is C,
LGPL/GPL, compiled and depends on libunistring. No header-only C++ IDNA2008
library exists on GitHub (section 3). **Recommendation: generate webcpp's own
tables**; section 6.

## 1. Method

`$S/suite.py` walks every string test of `idn-hostname`, `idn-email`,
`hostname` and `email` in `draft7`, `draft2019-09`, `draft2020-12`
(`optional/format/`) and `v1` (`format/`, where they are required); non-string
tests are ignored (formats ignore them). The data of the four drafts is
identical (checked: same `(data, valid)` lists), except v1's `idn-email`,
which repeats `"2962"` once more (19 instead of 18). Each string goes to a
harness (`$S/harness.cpp`) as `<format>\t<hex>` on stdin; the harness answers
`1` or `0 <reason>`, and `$S/report.py` counts per draft and file.

The harness judges:

- **idn-hostname**: `ada::idna::to_ascii` (UTS 46 mapping, NFC, per-label
  validity), then on the ASCII result the UTS 46 flags ada hard-codes off for
  WHATWG: `UseSTD3ASCIIRules` (LDH only), `VerifyDnsLength` (no empty label,
  so no leading or trailing dot; label at most 63 octets; name at most 253),
  `CheckHyphens` (no leading or trailing hyphen, no `--` in positions 3-4).
  Every `xn--` label is then decoded and checked with ada's own API
  (`punycode_to_utf32`, not all-ASCII, `map` stable, `normalize` stable,
  `is_label_valid`, CheckHyphens on the U-label). This is needed because
  `to_ascii` on an all-ASCII input returns it lowercased without looking at
  it (WHATWG `beStrict=false`, commit `ecd6572`), so `xn--X` would pass.
- **hostname**: non-ASCII rejected, then the same ASCII checks and A-label
  decoding (CheckHyphens on U-labels only, so `a--b.com` stays valid).
- **idn-email**: split on the last `@`; the local part per RFC 6531 3.3
  (Dot-string of `atext` or `UTF8-non-ascii`, or a Quoted-string of
  `qtextSMTP`, `quoted-pairSMTP` or `UTF8-non-ascii`; at most 64 octets); the
  domain is an address literal (`[IPv4]` or `[IPv6:...]`, via `inet_pton`) or
  an idn-hostname.
- **email**: the same, ASCII only, domain as a hostname.

Other backends replace `to_ascii` and the A-label check with
`upa::idna::to_ascii` (all flags), `idn2_lookup_u8` (optionally followed by
`idn2_register_u8(NULL, alabel)` per A-label) or ICU's
`uidna_nameToASCII_UTF8`; the email grammar and the DNS checks are shared.
The `layer` mode adds to ada, without tables of its own: Punycode re-encode
round trip, the RFC 5892 DISALLOWED exceptions as a list, CONTEXTO with
hand-written script ranges, and a crude cross-label Bidi rule 1.

Commands (all in `$S/run.sh`):

```sh
cd $S && unset CPATH CPLUS_INCLUDE_PATH C_INCLUDE_PATH
(cd idna && python3 singleheader/amalgamate.py) && cp idna/singleheader/ada_idna.{h,cpp} .
clang++ -std=c++20 -O2 -I. harness.cpp ada_idna.cpp -o harness
python3 suite.py "./harness ada" out_ada.json && python3 report.py out_ada.json
SDK=~/dev/webcpp/.local/wasi-sdk/bin
$SDK/clang++ --target=wasm32-wasip2 -fno-exceptions -std=c++20 -O2 -I. harness.cpp ada_idna.cpp -o harness_p2.wasm
$SDK/clang++ --target=wasm32-wasip3 -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false -std=c++20 -O2 -I. harness.cpp ada_idna.cpp -o harness_p3.wasm -lunwind
python3 suite.py "wasmtime run harness_p2.wasm ada" out_p2.json
python3 suite.py "wasmtime run -W exceptions=y harness_p3.wasm ada" out_p3.json
```

## 2. ada-url/idna

### 2.1 What it is

| Fact | Value |
| --- | --- |
| Licence | Apache-2.0 or MIT, at the user's choice (`LICENSE-APACHE`, `LICENSE-MIT`) |
| Version | 0.6.0 (CMake), HEAD `92ea246` is 28 commits past `v0.6.0` and fixes bugs the suite sees (section 2.3) |
| Standard | C++20 (README; `std::ranges`, `<bit>`, `<span>`, `starts_with`). `-std=c++17` fails with 21 errors; `singleheader/README.md` still says C++17 |
| Distribution | `src/*.cpp` + `include/ada/idna/*.h` built by CMake, or an amalgamation `singleheader/amalgamate.py` writes: `ada_idna.h` (208 lines) + `ada_idna.cpp` (6,642 lines, 412,834 bytes, of which the table blob `table_blob.inc` is 318 KB of source) |
| Header-only | **No.** Including `ada_idna.cpp` in two translation units gives 26 duplicate symbols; it would need every function made `inline` |
| Public API bug | `is_ascii` is declared `constexpr` in the header and defined in the `.cpp`: calling it from outside is an undefined symbol |
| Exceptions | Throws nothing itself; compiles and runs with `-fno-exceptions` (native, wasip2, wasip3). `std::string` growth under `-fno-exceptions` aborts on out-of-memory |
| Warnings | clean with `-Wall -Wextra`; 21 `-Wsign-conversion` with `-Wconversion` |
| Dependencies | none (optional simdutf, off by default) |
| Tables | Unicode/IDNA 17.0.0: UTS 46 mapping (two-stage), NFC decomposition/composition/CCC, Bidi class ranges, combining-mark ranges, ID_Start/ID_Continue (for ada's URL pattern, unused by IDNA). Stored as one raw-DEFLATE blob, 62,377 bytes compressed, 224,608 bytes inflated |
| Global state | Inflates the blob on first use into a `new (std::nothrow)` heap buffer that is never freed; publishes it through `inline` namespace-scope pointers and an `inline std::atomic<uint8_t>` state with a CAS spin-wait. Mutable static state by design |

API (`ada::idna`): `to_ascii(string_view) -> string` (empty on error) and
`bool to_ascii(string_view, string&)`; `to_unicode` (never fails: invalid
labels pass through); and the building blocks `map`, `normalize`,
`is_already_nfc`, `punycode_to_utf32`, `utf32_to_punycode`,
`verify_punycode`, `is_label_valid(u32string_view)`, UTF-8/UTF-32
transcoding, `valid_name_code_point` (identifiers). There are no options: the
processing is fixed to the WHATWG URL Standard's "domain to ASCII" with
`beStrict=false`.

### 2.2 What it implements

| Rule | ada |
| --- | --- |
| UTS 46 mapping, non-transitional | yes (deviations ß, ς, ZWJ, ZWNJ kept) |
| NFC | yes |
| UTS 46 CheckHyphens, UseSTD3ASCIIRules, VerifyDnsLength | **no** (WHATWG sets them false); the harness adds them |
| Label must not begin with a combining mark | yes |
| ACE label checks | only when the input has a non-ASCII code point; an all-ASCII input is returned lowercased |
| CONTEXTJ (ZWJ, ZWNJ) | **approximate**: virama from a hard-coded list; ZWNJ accepted when *any* L/D code point is anywhere before and *any* R/D anywhere after, from hard-coded lists (Arabic, Syriac, Mongolian, Phags-pa only), without the Joining_Type T adjacency of RFC 5892 A.1 |
| CONTEXTO (middle dot, Greek keraia, Hebrew geresh/gershayim, Katakana middle dot, Arabic-Indic digits) | **no** (UTS 46 does not have it); the digit mix is caught only through Bidi rule 4 |
| IDNA2008 derived property (RFC 5892 DISALLOWED exceptions, NV8/XV8 symbols) | **no**: UTS 46 validity only |
| Bidi rule (RFC 5893) | per label only, for labels that contain R, AL or AN; the "Bidi domain name" condition across labels (an LTR label of a Bidi name must also satisfy rules 1, 5, 6) is not applied |
| Punycode canonical round trip | yes at HEAD (`685f82f`, rejecting a leading delimiter) |

Beyond the suite (`$S/contextj_probe.py`), ada's CONTEXTJ is wrong in 4 of 5
cases where ICU is right in 5 of 5: NKo `U+07CA ZWNJ U+07CA` and Mandaic
`U+0841 ZWNJ U+0841` (both D ZWNJ D, valid) are rejected because NKo and
Mandaic are missing from the D list; `U+0628 U+0621 ZWNJ U+0628` and
`U+0628 ZWNJ U+0621 U+0628` (a non-joining HAMZA next to the ZWNJ, invalid)
are accepted because the rule is not anchored.

### 2.3 Results

| Draft | File | Total | Passed | Failed | Failed % |
| --- | --- | --- | --- | --- | --- |
| draft7 | idn-hostname | 92 | 77 | 15 | 16.3% |
| draft7 | idn-email | 18 | 18 | 0 | 0.0% |
| draft7 | hostname | 58 | 45 | 13 | 22.4% |
| draft7 | email | 65 | 65 | 0 | 0.0% |
| draft2019-09 | idn-hostname | 92 | 77 | 15 | 16.3% |
| draft2019-09 | idn-email | 18 | 18 | 0 | 0.0% |
| draft2019-09 | hostname | 58 | 45 | 13 | 22.4% |
| draft2019-09 | email | 65 | 65 | 0 | 0.0% |
| draft2020-12 | idn-hostname | 92 | 77 | 15 | 16.3% |
| draft2020-12 | idn-email | 18 | 18 | 0 | 0.0% |
| draft2020-12 | hostname | 58 | 45 | 13 | 22.4% |
| draft2020-12 | email | 65 | 65 | 0 | 0.0% |
| v1 (required) | idn-hostname | 92 | 77 | 15 | 16.3% |
| v1 (required) | idn-email | 19 | 19 | 0 | 0.0% |
| v1 (required) | hostname | 58 | 45 | 13 | 22.4% |
| v1 (required) | email | 65 | 65 | 0 | 0.0% |
| all four | idn-hostname | 368 | 308 | 60 | 16.3% |
| all four | idn-email | 73 | 73 | 0 | 0.0% |
| all four | idn-hostname + idn-email | 441 | 381 | 60 | 13.6% |
| draft7 to 2020-12 (optional) | idn-hostname + idn-email | 330 | 285 | 45 | 13.6% |
| v1 (required) | idn-hostname + idn-email | 111 | 96 | 15 | 13.5% |

The verdicts are identical natively with and without exceptions, on wasip2
(no exceptions), on wasip3 with wasm exceptions and on wasip3 without (the
`out_*.json` files compare equal).

Failing `idn-hostname` strings (each fails in all four drafts; all are
invalid strings ada accepts):

| String | Description | Rule ada lacks |
| --- | --- | --- |
| `실〮례.테스트` | contains illegal char U+302E Hangul single dot tone mark | RFC 5892 DISALLOWED exception (UTS 46 `valid NV8`) |
| `ـߺ` | Exceptions that are DISALLOWED, right-to-left chars | same |
| `〱〲〳〴〵〮〯〻` | Exceptions that are DISALLOWED, left-to-right chars | same |
| `a·l` | MIDDLE DOT with no preceding 'l' | CONTEXTO |
| `·l` | MIDDLE DOT with nothing preceding | CONTEXTO |
| `l·a` | MIDDLE DOT with no following 'l' | CONTEXTO |
| `l·` | MIDDLE DOT with nothing following | CONTEXTO |
| `α͵S` | Greek KERAIA not followed by Greek | CONTEXTO |
| `α͵` | Greek KERAIA not followed by anything | CONTEXTO |
| `׳ב` | Hebrew GERESH not preceded by anything | CONTEXTO |
| `״ב` | Hebrew GERSHAYIM not preceded by anything | CONTEXTO |
| `def・abc` | KATAKANA MIDDLE DOT with no Hiragana, Katakana, or Han | CONTEXTO |
| `・` | KATAKANA MIDDLE DOT with no other characters | CONTEXTO |
| `0a.א` | Bidi domain name with a digit-first label is invalid | Bidi rule across labels |
| `xn--7a` | A-label that decodes to a disallowed code point is invalid | IDNA2008 derived property (U+00A1 is UTS 46 `valid NV8`) |

The `hostname` failures are the A-label twins of the first 13 (`xn--07jt112bpxg.xn--9t4b11yi5a`,
`xn--chb89f`, `xn--07jceefgh4c`, `xn--al-0ea`, `xn--l-fda`, `xn--la-0ea`,
`xn--l-gda`, `xn--S-jib3p`, `xn--wva3j`, `xn--5db1e`, `xn--5db3e`,
`xn--defabc-k64e`, `xn--vek`). ada's ASCII path does not break `hostname` or
`email`: every pure-ASCII test passes; the 13 failures are IDNA2008 rules on
decoded A-labels, the same gap as above. `idn-email` and `email` pass every
test (the domain cases are hyphen, dot and emptiness rules).

### 2.4 ada-url/ada

ada (the WHATWG URL library, 4.0.0) vendors the same code as
`src/ada_idna.cpp` + `include/ada/ada_idna.h`, an amalgamation of ada-url/idna
generated on 2026-07-12; 97 lines differ from HEAD's amalgamation (the
`is_already_nfc` composition-exclusion check and the Hangul LV+T fix among
them). Measured anyway, it fails two more strings than HEAD: 17/92 (18.5%),
adding `क्‌षx‌y` (ZWNJ must pass at every
occurrence; fixed by `4ed8208`) and `xn---9uc` (non-canonical Punycode; fixed
by `685f82f`). Same code, older; nothing to gain from taking ada itself.

## 3. Other candidates

| Candidate | Licence | Form | Tables | idn-hostname failed | Notes |
| --- | --- | --- | --- | --- | --- |
| upa-url/idna `1e47bfa` | BSD-2-Clause | C++17, compiled (`src/idna.cpp`, `idna_table.cpp` 227 KB, `nfc_table.cpp` 134 KB of source) | UTS 46 + NFC, Unicode 18.0 | 14/92 (15.2%) | Every UTS 46 flag available (`CheckHyphens`, `CheckBidi`, `CheckJoiners`, `UseSTD3ASCIIRules`, `VerifyDnsLength`), so its Bidi rule is applied across labels (`0a.א` passes); fails the 3 DISALLOWED exceptions, `xn--7a` and the 10 CONTEXTO strings |
| libidn2 2.3.8 lookup | GPL-2.0+ or LGPL-3.0+ (plus GPL-3.0+ tools) | C, compiled; `libidn2.a` 228 KB native; links libunistring (2 MB dylib) and libintl | IDNA2008 derived property, TR46 | 13/92 (14.1%) | `idn2_lookup_u8` checks CONTEXTJ but not CONTEXTO (RFC 5891 5.4) |
| libidn2 2.3.8 lookup + register | same | same | same | 2/92 (2.2%) | `idn2_register_u8` per A-label adds CONTEXTO and the full derived property; still misses `0a.א` (Bidi across labels) and `א0٠` (Bidi rule 4). Not built for WASI in this spike |
| ICU 78.3 UTS 46 | Unicode-3.0 | C/C++, compiled, ICU data | UTS 46, CONTEXTJ, CONTEXTO, Bidi | 4/92 (4.3%) with the harness's DNS rules; 8/92 in the earlier probe without them | Fails only the 3 DISALLOWED exceptions and `xn--7a` (UTS 46 NV8). Section 4.2 of the research already rejected ICU for the wasm artifacts on size |
| Sourcemeta Core (`core/src/core/idna`, `punycode`, `unicode`) | AGPL-3.0 or commercial | C++, compiled, tables generated at build time from the UCD | everything | (Blaze passes 100% on Bowtie) | Ideas only. 4,353 lines including the UCD code generators and a broader Unicode module |
| Header-only C++ IDNA2008 | - | - | - | - | None found: `gh search repos` for `idna`, `punycode idna`, `uts46` in C and C++ returns ada-url/idna, upa-url/idna, `SBKarr/libuidna` (ICU extract) and bindings to libidn/libidn2; a web search finds nothing more |

The pattern is structural. UTS 46 implementations (ada, upa, ICU) cannot pass
the RFC 5892 exceptions and NV8 symbols, because those are exactly where UTS
46 deliberately differs from IDNA2008. Only ICU and libidn2's registration
path add CONTEXTO.

## 4. Building ada's IDNA for WASI

| Build | Result |
| --- | --- |
| native, `-std=c++20 -fno-exceptions` | builds, verdicts identical |
| wasip2, `-fno-exceptions` | builds, runs under wasmtime, verdicts identical |
| wasip3, wasm exceptions + `-lunwind` | builds, runs with `-W exceptions=y`, verdicts identical |
| wasip3, `-fno-exceptions` | builds, runs, verdicts identical |
| header-only (the `.cpp` included in two TUs) | 26 duplicate symbols |

Size in a stripped release component (`$S/size/probe.cpp`: one `to_ascii`
call and a `printf`, against the same program without ada; `-Os`,
`--strip-all`):

| Target | Without ada | With ada | Cost | gzip -9 cost |
| --- | --- | --- | --- | --- |
| wasip2, `-ffunction-sections -fdata-sections --gc-sections` | 45,004 | 129,164 | **84,160** | 71,444 |
| wasip2, `-flto` | 44,217 | 126,366 | 82,149 | 70,863 |
| wasip3, sections + gc | 63,508 | 152,288 | 88,780 | 73,143 |
| wasip3, `-flto` | 60,272 | 145,961 | 85,689 | 71,901 |

About 62 KB of the cost is the compressed table blob, which is why gzip
barely helps; the rest is the inflater, NFC, Punycode and validity code. On
first use it also allocates **224,608 bytes** of heap for the inflated tables,
for the life of the process.

Allocations (`$S/alloc_probe.cpp`, counting `operator new`): an ASCII
domain, 0; the first non-ASCII call, 3 (224,736 bytes, the table buffer
included); later non-ASCII calls, 2 (a `u32string` for the mapped form and the
output growth, about 72-128 bytes); `punycode_to_utf32`, 1;
`is_label_valid`, 0. Every buffer is a `std::string` or `std::u32string` with
`std::allocator`; nothing takes an allocator. An allocator adapter would need:
(1) every internal `std::u32string` and the out-parameters templated on an
allocator or turned into caller-provided spans, a patch through all of
`to_ascii.cpp`, `mapping.cpp`, `normalization.cpp`, `punycode.cpp`; (2) the
table blob either inflated into a caller-provided 224,608-byte buffer, or
replaced by uncompressed `constexpr` tables in read-only data, which also
removes the static mutable state; (3) `ensure_tables` and its atomics gone. That
is a fork, not an adapter.

## 5. The alternative: webcpp's own tables

`$S/estimate.py` builds each table webcpp would generate from the Unicode
17.0.0 UCD, encodes it the smaller of two ways (a run list of `u32` starts and
values, or a two-stage trie with deduplicated 32-256 code point blocks), and
measures raw and DEFLATE sizes:

| Table | Entries | Raw bytes | DEFLATE bytes |
| --- | --- | --- | --- |
| UTS 46 status and mapping index | 8,201 runs | 49,206 | 20,164 |
| UTS 46 mapping strings (UTF-8) + offsets | 3,946 targets | 21,328 | 9,668 |
| IDNA2008 NV8/XV8 (valid in UTS 46, not PVALID) | 740 runs | 3,700 | 1,282 |
| Bidi class | 1,445 runs | 7,225 | 2,674 |
| Joining type | 932 runs | 4,660 | 1,623 |
| Scripts for CONTEXTO (Greek, Hebrew, Hiragana, Katakana, Han) | 171 runs | 855 | 404 |
| Canonical combining class (virama = 9, NFC) | 606 runs | 3,030 | 1,342 |
| General category M (leading combining mark) | 655 runs | 3,275 | 1,115 |
| NFC canonical decompositions | 2,081 | 20,832 | 9,519 |
| NFC primary compositions | 961 | 11,532 | 4,203 |
| **Everything** | | **125,643** | **51,482** |
| Only what ada lacks (NV8/XV8, scripts, joining type, bidi) | | 16,440 | 4,840 |

The encodings are naive (the mapping index stores one value per mapped code
point; a delta encoding for case pairs would shrink it); ada fits mapping +
NFC + bidi + identifiers in 224,608 bytes raw because it optimises for O(1)
lookup, and deflates to 62,377. Stored as `constexpr` arrays in read-only data,
with no inflate, no heap and no static mutable state, webcpp's tables would
cost about 100-130 KB in the artifact, against ada's 84 KB of binary plus 225
KB of heap. `NV8`/`XV8` in `IdnaMappingTable.txt` gives the IDNA2008
derived-property difference directly (it contains U+00A1, U+0640, U+07FA,
U+302E..3035 and U+303B), so the IANA tables and RFC 5892's derivation need
not be reimplemented; the IANA registry stops at 12.0.0 anyway.

Code, by comparison with ada (2,406 lines of C++ outside the tables, identifier code included, plus
1,013 lines of Python generators) and Sourcemeta Core (507 idna + 342
punycode + 175 nfc + about 1,100 generator lines): Punycode about 250 lines,
UTS 46 mapping about 100, NFC about 300, validity (CheckHyphens, STD3,
lengths, leading mark, CONTEXTJ with joining types, CONTEXTO, NV8/XV8, Bidi
with the cross-label condition) about 350, the label driver and ASCII/Unicode
entry points about 200: **about 1,200 lines of header C++ and a 600-800 line
generator**, plus the unit tests. This agrees with the "~3k IDNA + tables"
of the research's section 7, row 6.

## 6. Verdict and recommendation

**Verdict against the 10% rule.** ada-url/idna fails 15 of 92
`idn-hostname` tests (16.3%) in every draft, required (v1) and optional
(draft7, 2019-09, 2020-12) alike; with `idn-email` (0 of 18-19 failed) it fails
60 of 441 (13.6%). **Over 10%: look elsewhere.** ada-url/ada's copy is the
same code and older (18.5%). Elsewhere: upa-url/idna 15.2% and libidn2's
lookup 14.1% are over too; libidn2 with registration (2.2%) and ICU (4.3%)
are under, but one is a C, LGPL/GPL library with libunistring and the other is
ICU, both compiled dependencies the research already ruled out for the wasm
artifacts. No header-only C++ IDNA2008 library exists.

The gap is not fixable from outside cheaply. A 60-line layer on ada's public
API passes 91 of 92 (`$S/harness.cpp`, mode `layer`), but it is fitted to the
suite: correct CONTEXTO needs the Script property, the IDNA2008 difference
needs the NV8/XV8 table, the cross-label Bidi rule needs the Bidi class (which
ada keeps private), and ada's own CONTEXTJ is wrong for NKo, Mandaic and
non-joining neighbours, so it would have to be replaced with a joining-type
table. Those four tables are 16 KB raw that webcpp must generate anyway, on
top of a dependency with static mutable state, a 225 KB heap buffer, no
allocator control, no header-only form and fixed WHATWG options.

**Recommendation: generate our own tables.** A `webcpp` IDNA header with
tables generated from `IdnaMappingTable.txt` (status, mapping, NV8/XV8),
`DerivedBidiClass.txt`, `DerivedJoiningType.txt`, `Scripts.txt`,
`DerivedCombiningClass.txt`, `UnicodeData.txt` and
`DerivedNormalizationProps.txt`, committed as generated files with a b2
target that regenerates them and a check that they match; `constexpr`
read-only data, no heap, no static state, errors as values; shared with the
regex engine's Unicode tables where they overlap. Use ada-url/idna (Apache-2.0
or MIT) as a reference for the algorithms and the table packing (its
`scripts/idna_table.py`, `pack_tables.py`), and its test vectors, not as a
dependency. Vendoring ada through a mirror repository and a submodule, as was
decided for SRELL, is not recommended: it would fail the rule as shipped, and
the patches needed to pass it and to fit webcpp's rules amount to a fork.
