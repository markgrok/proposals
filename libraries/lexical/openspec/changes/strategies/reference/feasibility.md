---
title: "Boost.Regex as webcpp's ECMA-262 engine: what extension can and cannot do"
tags: [regex, boost, json-schema, research]
status: active
created: 2026-10-09
---

# Boost.Regex as webcpp's ECMA-262 engine: what extension can and cannot do

**Question (the owner's).** Can Boost.Regex be specialized, and what it lacks implemented around
it, so that webcpp uses it instead of SRELL? Measured on 2026-10-09 against the requirements of
`~/.buzz/PLANS/WEBCPP_FORMATS_SPEC.md` (5.12, 7) and `~/.buzz/PLANS/WEBCPP_JSON_SCHEMA_SPEC.md`
(D11, S3, the regex policy of 4.2, risk 1 of 21): ECMA-262 `u`-mode semantics for `pattern` and
`format: regex`, every webcpp target, error values (never an abort) for a bad pattern and for a
runaway search, header-only, and every allocation through a caller's `std::pmr::memory_resource`.

**Short answer.** Extension gets much further than the earlier spike did: a `char32_t` traits
class with generated Unicode 17 tables, an ECMA-262 translator and a step-budgeted UTF-8 iterator,
with Boost.Regex 1.92 unmodified, pass **881 of 881** regex-dependent tests of the suite on all
five targets (native clang and gcc-15, wasip2 without exceptions, wasip3, emscripten), and turn
catastrophic backtracking into an error value even without exceptions. Three requirements are out
of reach without changing Boost: allocation through a `memory_resource` (Boost.Regex has no hook
for `basic_regex` nor for its backtracking stack), unbounded variable-width lookbehind (which the
maintainer has declined, twice), and ECMA-262's per-iteration capture reset. Two match-time aborts
remain by default and can be moved, not removed, by configuration. The verdict is **(c)** for the
whole requirement set. Fuzzing both engines against V8 along the way found that **upstream SRELL
2026.07 has a regression on a common path** (`(?:ab){1,2}` does not match `"ab"`, since 2026.06),
which the suite does not catch.

## Summary

| Requirement | Boost.Regex 1.92 as is | Boost.Regex + this prototype (no Boost change) | SRELL 2026.07 (+ webcpp mirror patches) |
| --- | --- | --- | --- |
| Suite, 881 regex-dependent tests, drafts 4 to v1 | 18.7% fail (narrow), 12.7% (`u32regex`+ICU), earlier spike | **0% fail** in every draft, required and optional, on all 5 targets | 0% (with `SRELL_NO_UBMOD`) |
| ECMA-262 beyond the suite: 87,028 queries of a grammar fuzz, answers equal to V8 | not measured | 96.04%; every difference is an `unsupported` error value (3,437) or a V8 deviation (6); **0 wrong answers** | 99.59%; 159 compile disagreements, 185 refusals, **4 wrong answers** (3 from a 2026.06 regression) |
| `format: regex` (syntax only) on 20,130 fuzz patterns | accepts perl syntax (spike) | **identical to V8 on all 20,130** | 159 differ (`\A \Z \z` accepted, quantified zero-width groups refused) |
| Unicode properties (`gc`, `sc`, `scx`, 53 binary) | narrow: none; ICU only via `u32regex` | yes, Unicode 17, generated, no ICU, no `std::locale` | yes, Unicode 18 |
| Variable-width lookbehind | refused, any width difference | leftmost quantifier and top-level alternatives handled; bounded is expandable; **unbounded non-leading impossible** | yes |
| Bad pattern as a value, no exceptions | yes (`no_except`, `status()`) | yes, from the translator (syntax) or Boost (`status()`) | yes (`quiet`, `ecode()`), after the mirror's no-exceptions patch |
| Runaway search as a value, no exceptions | **abort** (`raise_error` -> `throw_exception`) | **yes**, through the budgeted iterator (an unsupported mechanism, measured) | yes (`error_complexity` in `match_results::ecode()`) |
| Other match-time aborts, no exceptions | stack cap (4 MiB) and state cap (1e8) | the same two remain; stack cap removable by macro, state cap only movable, and on wasm32 not far enough | `bad_alloc` -> abort (the mirror's patch) |
| Header-only | yes since 1.76 | yes | yes |
| Every allocation through a `memory_resource` | no hook on `basic_regex`; stack blocks via `::operator new` and a global cache | **no**: 7 to 9 global allocations per compile, the backtracking stack global; only `match_results`' vector routes | yes, with the mirror's patch a8ecf39 (zero global allocations measured in its spike) |
| Minimal wasip2 program, stripped core module | - | 374 KB | 417 KB |
| Speed on the suite's 128 searches, per query (native) | - | compile 1.9 us, search 0.21 us | compile 5.0 us, search 0.04 us |
| Search on 1 MB instances | - | 1 to 16 ms | 0.02 to 5 ms (2x to 400x faster) |

## Verdict

- **(a) Boost.Regex as is plus our traits and translation layer, meeting every requirement: no.**
  It meets the suite (881/881 everywhere), the syntax check, header-only, every target, and the
  runaway-as-a-value requirement through the budget iterator. It cannot meet the allocation
  requirement, cannot run valid patterns with an unbounded non-leading lookbehind or a
  backreference into a quantified group (it refuses them as `unsupported`, 6.2% of the valid fuzz
  patterns), and keeps two configurable match-time aborts.
- **(b) Specific upstream changes would close it** (section 6): match-time error values (about
  100 to 150 lines), an allocator for `basic_regex` and the backtracking stack (300 to 600 lines,
  an API change), variable-width lookbehind (bounded 200 to 400 lines, unbounded over 1,000), the
  ECMA capture reset (50 to 150 lines), a 16-bit truncation fix (4 lines). The maintainer has
  declined variable-width lookbehind publicly (boostorg/regex#170, 2022; #243, 2025: "pretty much an
  abomination to implement"), so (b) is not a plan that webcpp controls.
- **(c) Cannot meet the requirements without a fork: yes, this is the verdict** for the full set.
  A fork means carrying 11,413 of Boost.Regex's 21,321 header lines (the matcher, parser, creator
  and their containers) with the changes above, against SRELL's 11,752-line header with webcpp's
  patches. Without the allocation and lookbehind requirements, Boost.Regex plus this prototype is
  a sound second engine behind the regex policy concept.

On the same criteria SRELL meets more today (lookbehind, error values, allocation by the existing
mirror patch, three to six times faster searches), but the regression found here is in a common
path and must be fixed or avoided before SRELL ships (section 7).

## 1. Inputs and method

| Input | Version |
| --- | --- |
| Boost | 1.92 headers, `/opt/homebrew/opt/boost/include` (`BOOST_VERSION 109200`, detail namespace `re_detail_600`) |
| Unicode data | UCD 17.0.0, downloaded from unicode.org; `UnicodeData.txt`, `Scripts.txt`, `DerivedNormalizationProps.txt` checked against the SHA-256 that webcpp's idna pins (`libs/idna/tools/unicode.json`) |
| SRELL | 2026.07 (SHA-256 `d384feae...ec20e`, the earlier spike's copy with its four `throw std::bad_alloc()` made `std::abort()`), 2026.06, 2026.05, 2026.04, 2026.03 from akenotsuki.com for bisection, and the webcpp mirror `~/dev/webcpp-infra/third_party/libs/srell` at 0b95517 |
| Suite | JSON-Schema-Test-Suite `5b0ee16`, the earlier spike's runner (`suite.py`, `report.py`) and V8 reference (`harness.mjs`), unchanged |
| Native | Apple clang 21.0.0 and g++-15.2.0, libc++/libstdc++, `-std=c++20 -O2` |
| WASI | wasi-sdk 34 (clang 23.1.0), wasmtime 47.0.3; wasip2 `-fno-exceptions -DBOOST_NO_EXCEPTIONS`; wasip3 `-fwasm-exceptions -mllvm -wasm-use-legacy-eh=false -lunwind`, run with `-W exceptions=y` |
| Emscripten | 6.0.11, `-fno-exceptions -DBOOST_NO_EXCEPTIONS`, run by node |
| Reference | node 26.7.0, V8 14.6.202.34, `new RegExp(p, "u")` |

Everything is under `~/.buzz/.scratch/boost-regex-ecma/` (called `$W`);
`$W/run.sh` rebuilds and re-measures all of it in about 4 minutes, its output in `$W/run.log`.

**The suite** is run exactly as in the earlier spike (`~/.buzz/RESEARCH/JSON_SCHEMA_ENGINE_SPIKE.md`
section 1): 885 regex-dependent tests, 4 skipped (`$recursiveAnchor`), 142 queries.

**The differential fuzz** (`$W/fuzz.py`, `$W/agg.py`) adds what the suite does not test: seeds 2
to 9, 3,000 random patterns each from a grammar of 51 atoms (properties, classes with negated
escapes, astral literals, `\u` forms, `\cJ`, `[]`, `[^]`), groups, named groups, lookarounds of
both directions, `(?i:` `(?m:` `(?s:` `(?-i:` modifiers, backreferences, anchors, `\b` `\B`, ten
quantifier forms, and in 30% of the patterns one of 41 injected defects (`\a`, `(?P<n>x)`, `\Z`,
`a{2,1}`, `(?<n>a)(?<n>b)`, ...). Each pattern is compiled and searched on 4 random subjects over
17 characters chosen for case folding and line terminators (`K`, `k`, U+212A, `s`, U+017F, U+2028,
U+00A0, U+1F432, ...). 111,671 queries, 20,130 patterns, 87,028 judged against V8.

## 2. Boost.Regex's extension points, read in the sources

Paths are relative to `/opt/homebrew/opt/boost/include/boost/regex/`.

### 2.1 The traits class

- `basic_regex<charT, traits>` (`v5/basic_regex.hpp:329-331`) takes any traits class that meets
  the documented requirements; with a nested `boost_extensions_tag` the wrapper uses the class's
  own `syntax_type`, `escape_syntax_type`, `error_string`, `toi` (`v5/regex_traits.hpp:51-60`,
  `:115-125`). `icu_regex_traits` (`v5/icu.hpp:125-365`) is the model of a non-locale traits.
- `char_class_type` can be a user type. Boost needs `= 0`, `!= 0`, `|=`, `&`, `==`
  (`v5/basic_regex_creator.hpp:73-74`, `:103-108`, `:381-389`; `v5/perl_matcher.hpp:245-247`).
  The prototype's is a 512-bit mask: 451 bits for the internal classes, 38 General_Category values,
  53 binary properties, 176 Script and 176 Script_Extensions values, and 3 case-closure flags.
- `\p{name}` and `\P{name}` reach `lookup_classname` with the text between the braces
  (`v5/basic_regex_parser.hpp:781-826`); inside a set only `[[:name:]]` and `[[:^name:]]` do
  (`v5/basic_regex_parser.hpp:1404-1430`, `:1471-1520`). The translator writes both forms with a
  bit number (`\p{#20}`, `[[:#20:]]`) after validating ECMA-262's names itself.
- The creator asks the traits for `w`, `s`, `lower`, `upper`, `alpha` and asserts each is
  non-zero (`v5/basic_regex_creator.hpp:276-289`); `\b` and `\B` read the `w` mask
  (`v5/perl_matcher_common.hpp:451-488`). With `w` as ECMA's ASCII word class, Boost's `\b`
  is ECMA-262's outside `(?i:`.
- **Negated classes inside one set are ORed into one mask and tested as "in none of them"**
  (`v5/perl_matcher.hpp:247`), where ECMA-262's `[\D\S]` means "outside any one of them". The
  translator gives each negated class escape its own alternative when a set has more than one.
- **`is_separator` truncates a wide character to 16 bits** (`v5/regex_traits_defaults.hpp:640-649`),
  so with `match_not_dot_newline` a `.` refuses U+12028, U+12029 and U+10085 (measured:
  `probe raw-dot-12028`, `raw-dot-10085`: "no match"; V8: match), and it treats `\f` and U+0085 as
  line terminators, which ECMA-262 does not. It affects `u32regex` too. The translator never emits
  a bare `.`.

### 2.2 The character type and the iterator

`u32regex` decodes UTF-8 with `u8_to_u32_iterator`, which calls `boost::throw_exception` on an
invalid sequence (`v5/unicode_iterator.hpp:676-681`): an abort without exceptions. The prototype
uses `basic_regex<char32_t, ecma::traits>` and its own bidirectional `u8_iterator` over the UTF-8
`std::string_view`, with no copy of the instance. The iterator is a template parameter of the public
API (`v5/regex_search.hpp:29-38`), which is what the budget of section 2.5 rests on.

### 2.3 Syntax and match options

`perl`, `ECMAScript` and `JavaScript` are the same value (`v5/regbase.hpp:92-94`); there is no
ECMA dialect to select. `no_mod_m` and `no_mod_s` (`:53`, `:56`) only stop inline modifiers; the
translator writes `\A`, `\z` and explicit sets for `^`, `$` and `.`, so no option carries ECMA
semantics. `no_except` (`:72`) governs the parser alone.

### 2.4 Allocation

| What | Through | Where |
| --- | --- | --- |
| the traits object | `new regex_traits_wrapper<traits>` | `v5/basic_regex.hpp:188` |
| the implementation | `new basic_regex_implementation`, held by `std::shared_ptr` | `v5/basic_regex.hpp:666-677` |
| the compiled program | `raw_storage`: `::operator new` | `v5/regex_raw_buffer.hpp:128`, `:207` |
| sub-expression positions, names | `std::vector` with `std::allocator` | `v5/basic_regex.hpp:168`, `:207-209` |
| lookbehind width computation | `std::vector` | `v5/basic_regex_creator.hpp:985` |
| `match_results`' sub-matches | **its `Allocator` parameter** (the only hook) | `v5/match_results.hpp:54`, `:588` |
| the backtracking stack | 4 KiB blocks from `::operator new`, up to 16 kept in a process-wide static cache | `v5/mem_block_cache.hpp:45-78`, `v5/perl_matcher_non_recursive.hpp:106-125`, `:231-248` |

`basic_regex` has no allocator parameter (`v5/basic_regex.hpp:329-331`). The static block cache
outlives any resource a search could have used; boostorg/regex#258 (open since 2025-08-09) shows it
holding pointers from a replaced `operator new` and corrupting memory. Measured in section 5.4.

### 2.5 Errors

- **Compile.** With `no_except`, a parse error is a `status()`; under `BOOST_NO_EXCEPTIONS`
  `regex_error::raise()` is empty anyway (`v5/pattern_except.hpp:62-71`). A search with a regex that
  failed to compile throws `std::invalid_argument` (`v5/perl_matcher_common.hpp:60-64`); measured on
  wasip2, an assertion abort (`basic_regex.hpp:634`). The prototype never searches with one.
- **Match.** Three sites call `raise_error` (`v5/perl_matcher_non_recursive.hpp:205-206`, recursion
  over 80; `:215-216`, `state_count > max_state_count`; `:246-247`, out of stack blocks), which
  always calls `raise_runtime_error` -> `boost::throw_exception` (`v5/pattern_except.hpp:82-98`).
  `boost::throw_exception` is declared `BOOST_NORETURN` (`boost/throw_exception.hpp:41-42`), so a
  user definition cannot return; there is no match flag, no `status()` on `match_results` and no
  non-throwing `regex_search`. In standalone mode it is `throw ex`, which does not compile without
  exceptions (boostorg/regex#279, open since 2026-05-21). **There is no supported way to get a
  value.**
- **Two unsupported ways, measured.**
  1. *Intercepting `raise_error` by argument-dependent lookup.* The calls are unqualified and
     `traits_inst` is a `regex_traits_wrapper<ecma::traits>` (`v5/perl_matcher.hpp:481`), so a
     non-template `ecma::raise_error(regex_traits_wrapper<ecma::traits> const&, error_type)` is
     found and preferred (`$W/probe.cpp`, `-DPROBE_ADL`: "intercepted raise_error(18)"). It is
     useless: after it returns, the matcher simply goes on (`(x+x+)+y` then ran 24.2 s and 13.4
     billion steps; the two others were still running at 60 s), and at the stack site the callers
     write below the block right after `extend_stack` returns (`:254-262`), a heap corruption.
  2. *A step budget in the iterator.* Each `*`, `++`, `--` and `==` of `ecma::u8_iterator` spends
     one step; when the budget is spent, every iterator of the search compares equal to every
     other, so each state finds itself at the end of the input and fails, the matcher unwinds
     normally, and `ecma::search` answers `limit`. It uses only public API, but it depends on how
     the matcher uses iterator equality, which Boost does not document. It works on every target
     (section 5.3).
- **Can a knob make the abort impossible?** `BOOST_REGEX_MAX_STATE_COUNT` (`config.hpp:291-292`)
  moves the state cap. For a non-random-access iterator it is the cap itself
  (`v5/perl_matcher_common.hpp:177-182`); for a random-access one it is
  max(N S^2 + 100,000, min(N^2 + 100,000, cap)) (`:106-175`). With a budget below the cap the
  catastrophic cases never reach it (measured: Boost's own limit fires after 161 to 310 million
  steps of the iterator, the budget after 10 million). But the cap is fixed for the bidirectional
  iterator, and a *legitimate* failing search on a long instance with a deeply nested pattern
  reaches it: `^(?:((((((((((((((((((((a))))))))))))))))))))|b)*c` on 3 MB of `a` throws
  `error_complexity` (an abort without exceptions) where the length-scaled budget has not run out.
  On 64-bit targets the macro can be raised past any reachable count; on wasm32 the counter is a
  32-bit `std::ptrdiff_t` (`v5/perl_matcher.hpp:487-489`) and 2^31 is reachable, so it cannot be
  made unreachable there. `BOOST_REGEX_MAX_BLOCKS` (`config.hpp:352-354`, 1,024 blocks of 4 KiB)
  is the stack cap: raised, the stack only ends at allocation failure, which aborts as SRELL's
  `bad_alloc` does. Recursion over 80 (`:205`) needs `(?>...)` or a conditional on an assertion
  (`v5/perl_matcher_non_recursive.hpp:417-427`, `:500-517`); the translator emits neither, so that
  site is unreachable.

## 3. The prototype

`$W/ecma_regex.hpp` (1,233 lines) and `$W/ucd_tables.hpp` (generated, 372 KB of source, 173 KB of
data in a wasm module) by `$W/gen_tables.py` (220 lines, after the method of webcpp idna's
`tools/generate_tables.py`: pinned UCD files, runs and ranges, no date in the output).

- **`ecma::traits`**: `char_type = char32_t`, `char_class_type = class_mask` (512 bits);
  `lookup_classname` knows Boost's five internal names and `#n`; `isctype` tests each set bit
  (General_Category by a 4,144-run table and a group mask, Script by 1,717 runs, Script_Extensions
  by 118 extension sets, binary properties by 12,606 ranges); `translate_nocase` is simple case
  folding (CaseFolding C+S, 1,512 pairs); three flag bits make a class match by case-folding orbit,
  for ECMA-262's ignoreCase (`(?i:\p{Lu})` matches `b`, `(?i:\W)` refuses U+017F, as V8).
- **`ecma::translate`**: a recursive-descent parser of Pattern[+UnicodeMode] (no Annex B) that
  refuses every u-mode early error the fuzz exercises and writes Boost syntax: literals as
  `\x{...}`; `^ $ .` per the `m`/`s` modifiers; `\d \w \s` and negations as ASCII or ECMA sets;
  `\p{...}` validated against ECMA-262's names and PropertyValueAliases (`\p{digit}` is `gc=Nd`, as
  in V8); classes with `[]`, `[^]`, ranges, and the negated-escape rule of 2.1; `\N` and `\k<n>`
  as `(?(N)\N)` (a reference to a group that has not matched matches empty, as ECMA-262 says;
  Boost's `\N` would fail); named groups as numbered ones; ES2025 modifiers, `i` through Boost's
  `(?i:` and the traits' orbit classes, `\b \B` under `i` as lookarounds; lookbehind with its
  top-level alternatives split into lookbehinds of their own and its leftmost quantifier reduced
  to its minimum count (`(?<=a+)b` holds where `(?<=a)b` does), which Boost then accepts.
  It refuses as `unsupported` what Boost would run differently: a backreference to a group inside
  a quantified atom, and a lookbehind Boost still rejects. Not done: `v` mode (JSON Schema uses
  `u`), duplicate group names in different alternatives (ES2025; refused).
- **`ecma::is_regex`** is the translator alone: `format: regex` is a syntax question.
- **`ecma::search`** with a budget of 10,000,000 steps plus 64 per byte of the instance (a linear
  match costs about 6 steps per character; a fixed budget refused `^[a-z]+$` on 3 MB) and any
  `match_results` allocator.

Files: `$W/harness_bx.cpp` (the spike's protocol: engines `bx`, `bx-strict`, `srell`, and `bench`,
`translate` modes), `$W/probe.cpp` (limits, aborts, raw Boost behaviour), `$W/alloc_probe.cpp`,
`$W/throughput.cpp`, `$W/size/`, `$W/fuzz.py`, `$W/agg.py`, `$W/classify.py`, `$W/q.py` (one
pattern against V8, bx and SRELL, with the translation), `$W/srell-probe/` (the SRELL bisection).

## 4. The suite

`bx` answers `format: regex` with the translator; `bx-strict` also requires Boost to compile the
translation. Fail % = (failed + errors) / (total - skipped), as in the spike.

| Target | `bx` required / all, every draft | `bx-strict` | `srell` |
| --- | --- | --- | --- |
| native clang, exceptions | 0.0% / 0.0% | 0.0% / 0.0% | 0.0% / 0.0% |
| native clang, `-fno-exceptions` | 0.0% / 0.0% | 0.0% / 0.0% | 0.0% / 0.0% |
| native g++-15, `-fno-exceptions` (and with exceptions) | 0.0% / 0.0% | 0.0% / 0.0% | 0.0% / 0.0% |
| wasip2, no exceptions | 0.0% / 0.0% | 0.0% / 0.0% | 0.0% / 0.0% |
| wasip3, wasm exceptions | 0.0% / 0.0% | 0.0% / 0.0% | 0.0% / 0.0% |
| emscripten, no exceptions | 0.0% / 0.0% | 0.0% / 0.0% | 0.0% / 0.0% |

Per draft, `bx`, identical on every target, verdict by verdict:

| draft | req total | req pass | req fail+err | req skip | req fail % | all total | all pass | all fail+err | all skip | all fail % |
|---|---|---|---|---|---|---|---|---|---|---|
| draft4 | 31 | 31 | 0 | 0 | 0.0% | 117 | 117 | 0 | 0 | 0.0% |
| draft6 | 37 | 37 | 0 | 0 | 0.0% | 123 | 123 | 0 | 0 | 0.0% |
| draft7 | 37 | 37 | 0 | 0 | 0.0% | 137 | 137 | 0 | 0 | 0.0% |
| draft2019-09 | 69 | 65 | 0 | 4 | 0.0% | 169 | 165 | 0 | 4 | 0.0% |
| draft2020-12 | 70 | 70 | 0 | 0 | 0.0% | 170 | 170 | 0 | 0 | 0.0% |
| v1 | 83 | 83 | 0 | 0 | 0.0% | 169 | 169 | 0 | 0 | 0.0% |
| overall | 327 | 323 | 0 | 4 | 0.0% | 885 | 881 | 0 | 4 | 0.0% |

**Remaining failures: none.** The earlier spike's last Boost failure, `(?<=a+)b` for
`format: regex`, passes in `bx` (syntax) and in `bx-strict` (the leftmost-quantifier reduction).

```sh
W=~/.buzz/.scratch/boost-regex-ecma; I=/opt/homebrew/opt/boost/include
SDK=~/dev/webcpp/.local/wasi-sdk/bin; cd $W
python3 gen_tables.py ucd_tables.hpp
clang++ -std=c++20 -O2 -DWITH_SRELL -DSRELL_NO_UBMOD -I$I harness_bx.cpp -o harness_native
$SDK/clang++ --target=wasm32-wasip2 -fno-exceptions -DBOOST_NO_EXCEPTIONS -DSRELL_NO_THROW -std=c++20 -O2 \
    -DWITH_SRELL -DSRELL_NO_UBMOD -I$I harness_bx.cpp -o harness_p2.wasm
python3 suite.py ./harness_native "bx,bx-strict,srell,v8-u@node harness.mjs" out_native.json
python3 suite.py "wasmtime run harness_p2.wasm" "bx,bx-strict,srell" out_p2.json
python3 report.py out_p2.json bx,bx-strict,srell
```

## 5. Beyond the suite

### 5.1 Differential fuzz against V8

`python3 agg.py "bx@./harness_native" "srell@./harness_native"` (the same on wasip2,
`"bx@wasmtime run harness_p2.wasm"`, gives the same `bx` numbers):

| Engine | Same as V8 | Differences |
| --- | --- | --- |
| `bx` | 83,585 of 87,028 (96.04%) | 1,617 `unsupported`: unbounded lookbehind; 874: bounded variable-width lookbehind; 750: backreference into a quantified group; 180: lookbehind alternatives of different widths inside a group; 16: backreference in a lookbehind; 6 V8 deviations that SRELL shares; **0 wrong answers**; **0 compile differences** over 20,130 patterns |
| `srell` 2026.07 | 86,674 of 87,028 (99.59%) | 112 compiles V8 refuses (all contain `\A`, `\Z` or `\z`); 47 compile refusals and 185 search errors (`error_badrepeat` 110 on a quantified group that holds only an assertion: `(?:^)?`, `(?-i:$){1,}`, `(?:\b)*`); **4 wrong answers**: 3 from the regression of 5.2 (`(?i:[\d\s]\cJ){1,2}?[\S\s]` on `"\n\n..."`), 1 older (a lookbehind holding a quantified named group and its backreference, which `bx` refuses; not analysed); the same 6 V8 deviations |
| `srell` 2026.05 | 86,677 of 87,028 | as 2026.07 but 1 wrong answer: the 3 of the regression of 5.2 are absent |

The 6 V8 deviations, checked by hand against ECMA-262 (both engines agree with the specification):
V8 14.6 matches `\B` between the two halves of a surrogate pair (`/\B/u.exec("S🐲1").index == 2`);
it lets a `(?i:...)` group change `\W` in another alternative (`/(?i:z)|^[^\W\d]/u.test("ſ")` is
true, without the group false); and `/[^\p{L}a]$/u.test("🐲")` is false.

Over the distinct valid patterns (`python3 classify.py`): 14,033 valid, 1,916 with a lookbehind;
`bx-strict` refuses 876 (6.2%): 412 unbounded lookbehind, 223 bounded variable-width, 46
alternatives of different widths inside a group, 4 backreference in a lookbehind, 191
backreference into a quantified group. No other construct is refused.

### 5.2 SRELL: a regression found by the fuzz

```cpp
srell::regex re("(?:ab){1,2}");
srell::regex_search("ab", re);   // false since SRELL 2026.06; true in 2026.05, 2026.04, 2026.03 and in V8
```

Reproduced with the pristine 2026.07 archive (clang `-O0` and `-O2`, g++-15, the single-header
build), with 2026.06, and with the webcpp mirror at 0b95517 (`$W/srell-probe/t.cpp`, `t2.cpp`).
It needs a non-capturing group with more than one atom, a bounded `{m,n}` with `m >= 1` and
`n > m`, and exactly `m` repetitions in the input: `(?:ab){1,2}` on `"xab"`, `(?:abc){1,2}`,
`(?:a[b]){1,2}`, `(?:ab){2,3}` on `"abab"` fail; `(ab){1,2}`, `(?:ab){1,}`, `(?:ab)+`,
`(?:ab){0,2}`, `(?:a){1,2}` and `(?:ab){1,2}` on `"abab"` match. A realistic schema pattern hit by
it: `^(?:[0-9]{3}-){1,2}[0-9]{4}$` refuses `"123-4567"`. The 2026.06 history entry ("not to check
beyond a repeated group when deciding a starting character set") is the likely cause. The suite
does not exercise it, which is why 881/881 hid it.

### 5.3 Runaway searches, limits and aborts

`$W/probe.cpp`, one case per run; native with exceptions (`probe_exc`), native without
(`probe_noexc`), wasip2 without (`probe_p2.wasm`), wasip3 and emscripten:

| Case | Prototype, budget (every target) | Boost's own limit, budget unlimited: exceptions | the same without exceptions (native, wasip2, emscripten) |
| --- | --- | --- | --- |
| `(a*)*b` on 40 `a` | `limit`, 0.022 s native, 0.043 s wasip2, 0.066 s emscripten | `error_complexity` thrown after 0.42 s, 161.5 M steps | **abort** (exit 134; wasip2 exit 101/134) |
| `(x+x+)+y` on 30 `x` | `limit`, 0.019 s | thrown after 0.41 s, 208.3 M steps | **abort** |
| `^(\w+\s?)*$` on a 72-char sentence | `limit`, 0.022 s | thrown after 0.68 s, 309.7 M steps | **abort** |
| `^(a\|bc)*$` on 30,000 `a` (60,000 on wasm32) | `found` | `found` | `found` |
| `^(a\|bc)*$` on 100,000 `a` | **abort**: `error_stack`, the 4 MiB stack cap (native after 369,005 steps, about 36,900 characters; wasm32 between 60,000 and 100,000), on every target | thrown | **abort** |
| the same, `-DBOOST_REGEX_MAX_BLOCKS=1073741824` | `found` on 900,000 `a` (106 MB resident) | - | - |
| `^(?:((((((((((a))))))))))\|bc)*$`, 300,000 `a`, blocks raised | `found`, 191 MB resident (SRELL: 189 MB) | - | - |
| `^(?:(...20 nested groups...a...)\|b)*c` on 3 MB, blocks raised | **`error_complexity`** where the length-scaled budget is not spent | thrown after 0.93 s | abort (the same `raise_error` path; not run) |

So the budget turns runaway time into a value everywhere; the stack cap needs
`BOOST_REGEX_MAX_BLOCKS` raised (then memory is bounded only by the budget times the pattern's
capture nesting, about 11 bytes per step for `^(a|bc)*$` and 63 for ten nested captures, the same
order as SRELL); and the fixed state cap of a non-random-access iterator can still abort a long
legitimate search. A random-access iterator over a decoded `char32_t` copy of the instance (from
the validation's resource) would get Boost's length-scaled cap instead; not built here.

### 5.4 Allocation

`$W/alloc_probe.cpp`: global `operator new` replaced and counting; a counting
`std::pmr::memory_resource` backed by `malloc` given to `match_results` as
`std::pmr::polymorphic_allocator<sub_match<u8_iterator>>`; `-fno-exceptions`.

| Phase | global `new`: count | bytes | resource: count | bytes |
| --- | --- | --- | --- | --- |
| `^\p{Letter}+$`: compile (of which Boost's `basic_regex`) | 13 (7) | 1,977 (1,633) | 0 | 0 |
| first search on 2 chars | 1 (a 4 KiB stack block) | 4,096 | 1 | 120 |
| second search | 0 (the block comes from the static cache) | 0 | 1 | 120 |
| `(?<year>\d{4})-(?<month>\d{2})`: compile (Boost's) | 22 (9) | 4,605 (3,785) | 0 | 0 |
| its searches | 0 | 0 | 1 | 200 |
| `^(a\|bc)*$` on 1,000 `a`: first, second search | 27, 12 | 110,592, 49,152 | 1, 1 | 160 |
| `^(a\|bc)*$` on 30,000 `a`: each search | 818 | 3,350,528 | 1 | 160 |
| `^[a-z]+$` on 100,000 `a`: each search | 0 | 0 | 1 | 120 |

The translator's own allocations (the rest of each compile) could take the resource; Boost's 7 to
9 per compile and its backtracking stack cannot, without a patch. A replaced global `operator new`
reaches them, but that is process-wide, not a caller's resource, and the static block cache keeps
blocks across calls (#258).

### 5.5 Size

Minimal wasip2 program: compile one pattern from `argv`, search once, print (`$W/size/`).

| Program | wasip2 component as linked (`-O2`, debug sections as the toolchain emits them) | core module, `llvm-strip`ped (`-O2` / `-Os`) | of which code / data |
| --- | --- | --- | --- |
| empty (`printf` only) | 278 KB | 27 KB | 23 KB / 3 KB |
| Boost.Regex + prototype | 1,257 KB | 374 KB / 327 KB | 199 KB / 173 KB |
| SRELL 2026.07 | 705 KB | 417 KB / 396 KB | 104 KB / 312 KB |

The earlier spike's 3.6 MB for narrow `boost::regex` came from `cpp_regex_traits` (`std::locale`,
iostreams); `ecma::traits` uses neither.

### 5.6 Speed

The suite's 128 search queries, compile and search timed apart, best of 20 rounds
(`harness bench <engine> 20 < suite_queries.txt`):

| | compile per query | search per query |
| --- | --- | --- |
| prototype, native | 1.85 us | 0.21 us |
| SRELL, native | 5.03 us | 0.04 us |
| prototype, wasip2 | 2.78 us | 0.49 us |
| SRELL, wasip2 | 5.69 us | 0.15 us |

One search on a 1 MB instance (`throughput.cpp`, best of 5; native, wasip2 in brackets):

| Pattern | prototype | SRELL |
| --- | --- | --- |
| `^[\p{L}\d ]+$` (match) | 10.6 ms (15.4) | 4.9 ms (5.2) |
| `\d{4}-\d{2}-\d{2}$` (at the end) | 6.6 ms (10.3) | 0.02 ms (0.11) |
| `^(?:[A-Za-z0-9+/]{4})*$` | 15.5 ms (22.7) | 1.9 ms (2.0) |
| `(?<year>\d{4})-(?<month>\d{2})-(?<day>\d{2})` | 8.4 ms (12.8) | 0.02 ms (0.10) |
| `x` (absent) | 1.1 ms (1.4) | 0.02 ms (0.13) |

Boost compiles faster; SRELL searches 2x to 400x faster: it scans for a required character with
`memchr`, while Boost on a bidirectional decoding iterator steps through every code point and the
budget accounting costs on every iterator operation.

## 6. What extension cannot fix, and the smallest fix for each

| Gap | Measured | Smallest fix | Size (estimate) | Plausible upstream? |
| --- | --- | --- | --- | --- |
| Match-time error value (complexity, stack) | aborts without exceptions (5.3); budget iterator works around time, not the two caps | a non-throwing search: record the code at the three `raise_error` sites, stop the matcher, report it (a flag plus `match_results::status()`, or an overload with `error_type&`); stack exhaustion as the same error | 100 to 150 lines and tests | possible: no-exceptions use is acknowledged (#279 open), no maintainer answer yet |
| Allocation through a `memory_resource` | 7 to 9 global allocations per compile, global stack blocks (5.4) | an allocator (or `memory_resource*`) for `basic_regex` (`raw_storage`, the vectors, `allocate_shared`, the traits wrapper), and the stack blocks from `match_results`' allocator instead of the static cache | 300 to 600 lines, an API change | low to moderate; the cache part also fixes #258 |
| Variable-width lookbehind | refused (`v5/basic_regex_creator.hpp:920-943`, `:978-1096`: one width, equal across alternatives) | bounded: compute [min, max] and try each width; unbounded: match the lookbehind backwards (V8, SRELL) | bounded 200 to 400 lines; unbounded over 1,000 | **no**: "never supported" and "not easy to implement, and I don't have time" (#170, 2022-01-11, 2022-10-03); "We don't support that... Variable length lookbehind is pretty much an abomination to implement IMO" (#243, 2025-02-06). Without Boost: bounded widths expand into fixed-width lookbehinds (`(?:(?<=a)\|(?<=aa))b` compiles and matches, `probe raw-lookbehind-expanded`); unbounded non-leading, such as `(?<=\$\d*)x`, is impossible |
| Captures reset at each iteration (ECMA-262 RepeatMatcher) | `^(?:(a)\|b)+\1$` on `ab`: V8 true, Boost false; refused as `unsupported` | a syntax option that saves and clears the captures inside a repeat at each iteration | 50 to 150 lines | low: ECMAScript is documented as a synonym of perl |
| `is_separator` truncation | `.` refuses U+12028 (2.1) | compare the full code point | 4 lines | yes, a plain bug |
| Fixed state cap for bidirectional iterators | aborts a long legitimate search (5.3) | a per-call cap, or scale with the input as for random access | 20 to 40 lines | possible |

## 7. SRELL on the same criteria

| Criterion | SRELL 2026.07 | Boost.Regex + prototype |
| --- | --- | --- |
| Suite | 881/881 with `SRELL_NO_UBMOD` | 881/881 |
| ECMA-262 coverage | lookbehind of any width, `v` mode, modifiers, named groups including duplicates | no unbounded non-leading lookbehind, no capture reset, no `v` mode; refused as values |
| Wrong answers in the fuzz | 4: 3 from the 2026.06 regression (5.2), 1 older case | 0 |
| Syntax check (`format: regex`) | accepts `\A \Z \z` (no macro to stop it), refuses quantified assertion-only groups; formats' spec already filters the first | identical to V8 on 20,130 patterns |
| Unicode | 18.0, shipped tables | 17.0, our generator (shared method with idna) |
| No exceptions | needs the mirror's patch for 4 `throw std::bad_alloc()` | compiles as is; needs `BOOST_NO_EXCEPTIONS` handler as webcpp has |
| Runaway as value | built in (`error_complexity`) | budget iterator, an undocumented dependency on the matcher |
| Other aborts | allocation failure | allocation failure, stack cap (macro), state cap (macro on 64-bit only) |
| Allocation through a resource | the mirror's patch a8ecf39 (+281/-106 lines), zero global allocations in its spike | not possible without a patch of Boost |
| Speed | compile slower, search 2x to 400x faster | compile faster, search slower |
| Size (stripped core) | 417 KB | 374 KB |
| Code webcpp owns | the mirror's patches over an 11,752-line header | 1,233 lines of prototype, a 220-line generator, on 21,321 lines of Boost.Regex |
| Upstream response | fixes credited to reporters in its history (2026.00, 2026.01) | lookbehind declined by the maintainer |

**SRELL's defects, by origin.** In upstream SRELL, recorded in
`~/dev/webcpp/.superpowers/sdd/WEBCPP_SHARED_INFRA_PLAN/srell-upstream-notes.md`: `SRELL_NO_APIEXT`
stops every program compiling; `regex_iterator2` over `std::string::const_iterator` does not
compile; assigning to and copying from a moved-from regex crash (fixed in the mirror by 67716ec).
Found here, upstream: the counted-repeat regression of 5.2 (2026.06 and 2026.07, a common path),
and the refusal of quantified groups that hold only an assertion (all versions tried, a rare path).
The other defects met so far were in webcpp's own allocation patch, recorded in the reviews of the
same plan (`task-5-review.md`, `task-5-rereview.md`), not in SRELL. Not re-measured here.

**Recommendation.** Keep SRELL as the engine, but pin a fixed version: report 5.2 upstream with
its reproduction, and until a fixed release, either take 2026.05 (measured: 881/881, and one wrong
answer left in the fuzz) or carry the fix as a mirror commit; add `(?:ab){1,2}` on `ab` and the
quantified-assertion cases to webcpp's regex tests, since the suite cannot catch them. Keep this
prototype as the basis of an optional Boost.Regex policy for users who already depend on Boost.Regex
and accept its documented gaps; it is the only Boost-based configuration measured at 881/881 without
ICU and with runaway searches as values on wasip2.

## 8. Commands

All from `$W`; `run.sh` runs them all.

```sh
W=~/.buzz/.scratch/boost-regex-ecma; cd $W
I=/opt/homebrew/opt/boost/include; SDK=~/dev/webcpp/.local/wasi-sdk/bin
P2="--target=wasm32-wasip2 -fno-exceptions -DBOOST_NO_EXCEPTIONS -DSRELL_NO_THROW"
P3="--target=wasm32-wasip3 -fwasm-exceptions -mllvm -wasm-use-legacy-eh=false"
python3 gen_tables.py ucd_tables.hpp
# suite on every target (section 4)
g++-15 -std=c++20 -O2 -fno-exceptions -DBOOST_NO_EXCEPTIONS -DSRELL_NO_THROW -DWITH_SRELL -DSRELL_NO_UBMOD \
    -I$I harness_bx.cpp -o harness_gcc_noexc
$SDK/clang++ $P3 -std=c++20 -O2 -DWITH_SRELL -DSRELL_NO_UBMOD -I$I harness_bx.cpp -o harness_p3.wasm -lunwind
source ~/dev/webcpp/.local/emsdk/emsdk_env.sh && em++ -std=c++20 -O2 -fno-exceptions -DBOOST_NO_EXCEPTIONS \
    -DSRELL_NO_THROW -DWITH_SRELL -DSRELL_NO_UBMOD -I$I -sNODERAWFS=1 -sALLOW_MEMORY_GROWTH=1 harness_bx.cpp -o harness_em.js
python3 suite.py "wasmtime run -W exceptions=y harness_p3.wasm" "bx,bx-strict,srell" out_p3.json
python3 suite.py "node harness_em.js" "bx,bx-strict,srell" out_em.json
# fuzz (5.1), one pattern by hand
python3 agg.py "bx@./harness_native" "srell@./harness_native"; python3 classify.py
python3 q.py '(?:ab){1,2}' ab
# SRELL bisection (5.2)
clang++ -std=c++20 -O2 -I<srell dir> srell-probe/t.cpp -o t && ./t
# limits and aborts (5.3)
clang++ -std=c++20 -O2 -fno-exceptions -DBOOST_NO_EXCEPTIONS -I$I probe.cpp -o probe_noexc
./probe_noexc catastrophic1; ./probe_noexc nobudget1; ./probe_noexc stack-cap 100000
clang++ -std=c++20 -O2 -fno-exceptions -DBOOST_NO_EXCEPTIONS -DPROBE_ADL -I$I probe.cpp -o probe_adl
$SDK/clang++ $P2 -std=c++20 -O2 -I$I probe.cpp -o probe_p2.wasm && wasmtime run probe_p2.wasm catastrophic1
# allocation, speed, size (5.4 to 5.6)
clang++ -std=c++20 -O2 -fno-exceptions -DBOOST_NO_EXCEPTIONS -I$I alloc_probe.cpp -o alloc_probe && ./alloc_probe
./harness_native bench bx 20 < suite_queries.txt; ./harness_native bench srell 20 < suite_queries.txt
clang++ -std=c++20 -O2 -DSRELL_NO_UBMOD -I$I throughput.cpp -o throughput && ./throughput
$SDK/clang++ --target=wasm32-wasip1 -fno-exceptions -DBOOST_NO_EXCEPTIONS -std=c++20 -O2 -I$I size/bx.cpp \
    -o size/bx_p1_O2.wasm && $SDK/llvm-strip size/bx_p1_O2.wasm -o size/bx_p1_O2.strip.wasm
```
