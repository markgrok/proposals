# webcpp::idna: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal took idna in as a retroactive genesis. `spec.md` and
`plan.md` are the specification and plan of 2026-10-09 (written when the consumer library was
named formats and is now named lexical); `../tasks.md` is the checklist; `source/` is the
library's tree at `e2452e5`, exactly `git archive e2452e5` of the library's repository (checked
with `diff -r` on 2026-10-13: identical, dotfiles included).

## Where it stands

The library was developed from 2026-10-09 to 2026-10-12, its repository's history 34 commits, all
as `R`, from `be969f4` ("the library's skeleton, its errors and its options") to `e2452e5` ("the
vectors' strings are sv literals, within every compiler's constant-evaluation steps"). Its
`origin` is `git@github.com:webcpporg/idna.git`.

| What | Where |
| --- | --- |
| The library | `libs/idna` of the superproject, at `e2452e5`; the repository `webcpporg/idna` |
| The superproject | its local branch `lib/formats-publish` records `libs/idna` at `e2452e5` since `dac26e2` (2026-10-13, "libs/idna and libs/formats: the submodules"); `05705ca` and the commits after it keep it there |
| The page | built by the library's CI from `doc/idna.adoc`; published at `webcpporg.github.io/libs/idna/` |

What the sources do not let me establish: whether the superproject commit that holds `libs/idna`
is on its public `main`. In the clone I read, the library's `origin/main` ref reads `a8024d1`
(five commits before `e2452e5`) and the superproject branch is five commits ahead of its
`origin/main`; both are remote-tracking refs that may be stale. The lexical proposal's
`reference/STATUS.md` records a superproject CI run at `05705ca` that built `libs/idna` with
`libs/lexical` (GCC 14 and 15, Clang 18 and 22, Apple Clang, wasm32-wasip2, wasm32-wasip3,
Emscripten 6.0.11, the documentation and the site: green; MSVC failed before building anything,
below). The release "0.1.0" is the metadata this proposal was given; the page's changelog still
lists idna's lines under "Unreleased".

### What was measured

By me, on 2026-10-13, outside b2 (a scratch directory, nothing written into any repository):
`clang++ -std=c++20 -O1` (Apple clang 21.0.0, macOS), Boost 1.92 from Homebrew, webcpp's
`libs/config/include` from the superproject checkout, with
`_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_EXTENSIVE`, each test built from `source/` and run:
`errors`, `options`, `utf8`, `punycode`, `properties`, `vectors`, `known_failures`, `nfc`,
`context`, `bidi`, `process`, `validity`, `uts46` and `allocation` all exited 0 and printed
`No errors detected.`, the timing tests among them. The four examples (`idna_quick_start`,
`idna_punycode`, `idna_validity`, `idna_allocation`) printed exactly their `.expected` files.
`test/config_test.cpp` compiled (syntax only) as built, with `-DWEBCPP_NO_EXCEPTIONS
-DWEBCPP_TEST_IDNA_ALL_OFF` and with `-DWEBCPP_IDNA_NO_EXCEPTIONS
-DWEBCPP_TEST_IDNA_DEVELOPER_DEFINED`. The generated vectors hold 6,391 IdnaTestV2 rows and 20,034
NormalizationTest rows (counted with `doc/counts.py`'s rule).

Not run by me: b2 and its lanes, wasm32-wasip2, wasm32-wasip3, Emscripten, GCC, MSVC, the
`run-fail` twins of `known_failures`, headers-alone, the own lane `tables` (it needs the network),
the lint, the page, and the compile-cost and stack figures below.

Stated in the sources, not seen by me:

- `doc/unicode.adoc`: the tables hold 147,324 bytes; a translation unit including
  `<webcpp/idna.hpp>` and calling `to_ascii` at `-O2` takes about 0.7 s and makes an object of
  188 KB natively with Apple Clang 21, and about 0.8 s and 197 KB for wasm32-wasip2 with
  wasi-sdk 34 (against 0.14 s and 0.07 s without idna; the least of fifteen compilations, on an
  Apple M5 Max).
- `doc/limits.adoc`: the deepest call ran in about 11 KiB of stack and an ordinary name in about
  8 KiB (wasi-sdk 34, `-O0 -g`, wasmtime 47, stack made smaller 256 bytes at a time until it
  trapped), against 64 KiB.
- `reference/plan.md`: the prototype passed all 6,391 lines of IdnaTestV2.txt in both passes and
  all 20,034 lines of NormalizationTest.txt, and JSON Schema's `idn-hostname` file
  (draft2020-12) 92 of 92 through `to_ascii` with the defaults; the library built by b2 passed
  every test and example on native Clang, GCC 15, wasip2, wasip3 and Emscripten, and the lint was
  clean.

## Open items

1. **MSVC.** Once the superproject loaded every Jamfile on Windows (`path.exists` given native
   paths, `1e33cbc` and `c53f3f9`), CI run 38365413330 (2026-10-13) ran idna on MSVC 14.3 and
   14.5 for the first time, and five tests fail on both: `uts46`, `validity` and `vectors` do
   not compile, since `test/generated/idna_test_v2.ipp` writes an octal escape followed by a
   decimal digit and MSVC's warning C4125 is an error there (the generator must close the
   string literal after such an escape); `nfc`'s `the_growing_edge` counts 2 and 3 allocations
   where it expects 0 and 1 (MSVC's strings keep fewer characters inline); `allocation`'s
   `pmr_forms_allocate_through_their_resource` sees 0 global allocations where the resource
   counted 225. The superproject's report marks them failures while the MSVC jobs pass.
2. **The former name in idna's own files.** `AGENTS.md` (lines 26 and 35), `doc/testing.adoc`
   (lines 131 and 132) and `meta/include-boundaries.json` (the entry `webcpp/formats`) still say
   `webcpp::formats`, the name lexical had until 2026-10-13. A change renames them to lexical
   (the boundary keeps forbidding the consumer's include).
3. **The Unicode data notice** (the plan's P8) was ruled provisionally in the plan (the notice in
   the page's appendix, one comment line in each generated file, no separate licence file) and
   put to the owner before publication; the sources I read do not record the owner's answer
   separately.
4. **Unicode 18.0.0** (UTS 46 revision 36) is out; the move is not made (a change of its own;
   `doc/unicode.adoc` lists every file it touches).
5. **Timing tests.** Four tests compare against a baseline in the same run (NFC's run of
   non-starters, `check_others`, `check_domain` over 65,536 labels, the pmr `to_ascii` of 16,384
   ideographs); they passed here, and the bounds leave room (the page notes a loaded machine took
   the CONTEXTO ratio to 41 against a bound of 1,000). A flake would be a defect to fix, not to
   retry.

## How to resume

1. Work in a worktree of the superproject (`git worktree add ../webcpp-idna-<slug> -b
   lib/idna-<slug> main`, then `git submodule update --init libs/idna libs/config`), following the
   superproject's `AGENTS.md`, chapter "Working on a library". `libs/idna` is the library's
   repository; `source/` is the same tree, for reading without a checkout.
2. Close the open items above first, item 2 being a change of text only.
3. Every change follows the library's own rules (`source/AGENTS.md`): tests first; the errors'
   values 1 to 19, the category's name and identity never change; the public interface of
   `errors.hpp`, `options.hpp`, `punycode.hpp`, `validity.hpp` and `uts46.hpp` is announced to
   lexical before it changes; no `throw` in a header; the generated files are never edited by
   hand (`b2 libs/idna/test//update-generated`); a name in the page's prose that holds `--` is
   written `` `+xn--bcher-kva.example+` ``; renaming a page section renames its `@see` lines.
4. Before saying a task is done: the lint, `b2 -a libs/idna/test libs/idna/example` natively and
   for `toolset=clang-wasip2 testing.launcher=wasmtime`, `toolset=clang-wasip3
   testing.launcher=wasmtime` and `toolset=emscripten`, `b2 -a libs/idna/test//tables` (network
   until the cache is filled), `b2 libs/idna/doc`, and then CI on every lane.
5. Commits carry the identity `R <github.overboard312@passmail.net>` and no trailer.

## What lexical needs from idna

`lexical` calls only idna's public interface: `idna::check_label` (one A-label at a time, for
`hostname`), `idna::to_ascii` into a stack buffer of `idna::max_domain_length` octets (for
`idn-hostname` and `idn-email`), and the `idna::error` values it returns as its rules' reasons
(`contexto`, `hyphen_at_start_or_end`, `bidi`). A wrong verdict found there becomes a red test in
idna, never a workaround in lexical.
