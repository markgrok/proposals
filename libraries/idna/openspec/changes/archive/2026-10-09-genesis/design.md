## Context

webcpp::idna was specified (`reference/spec.md`, approved on 2026-10-09) and planned task by task
(`reference/plan.md`, Tasks 0 to 14) as the library under the host-name and e-mail formats of
the library then named formats and now named lexical. It was built in 34 commits from
2026-10-09 to 2026-10-12, all as `R`, and entered the superproject as a submodule next to
lexical. `reference/source/` is its tree at `e2452e5`.

This document records, retroactively, the decisions 0.1.0 is built on. The specification
labelled some of them as the owner's (binding) and some as proposals for the owner's review; the
sources do not record the owner's answer to each proposal separately, and a proposal that the
library as published carries is recorded here as a decision of 0.1.0.

## Goals / Non-Goals

**Goals:**

- Implement Punycode, UTS 46 processing and IDNA2008 validity, and report for a name both whether
  it is valid and what its ASCII form is, exactly as the standards define them, proven by
  Unicode's conformance files.
- Be strict by default: the combination JSON Schema's `idn-hostname` asks for, which no UTS 46
  library gives alone.
- Allocate nothing on the validation path and in every span form, raise nothing, and use a fixed
  small stack, so that the library runs inside a WebAssembly program with the default 64 KiB
  stack.
- Run natively and on wasm32-wasip2, wasm32-wasip3 and Emscripten, with the same verdict for the
  same input on every target.
- Keep Unicode data out of hand-written code: every table and every conformance vector is
  generated from pinned files, and a check proves the committed files are the generator's output.

**Non-Goals:**

- IDNA2003, registry-specific rules (RFC 5891 section 4.3 leaves them to registries), confusable
  detection (UTS 39), and the WHATWG URL host parser; a later URL library may build on `to_ascii`
  with its own options.
- DNS resolution or any lookup: idna judges and converts strings only.
- A heap-based API on the validation path, or an exception-throwing one.

## Decisions

### D1. A library of webcpp's own, with Unicode's files as its evidence

idna ports nothing and has no oracle, no twins and no "Differences" appendix. Its evidence is
IdnaTestV2.txt and NormalizationTest.txt for Unicode 17.0.0 (UTS 46 revision 35), RFC 3492's
samples, the rules of RFC 5892 and RFC 5893 tested case by case, and, through lexical, the
official JSON Schema test suite. ada-url/idna is read as a reference for algorithms and table
packing only; nothing of it is copied. Alternatives rejected: wrapping ICU or libidn2 (compiled
dependencies, not header-only), or taking ada-url/idna as it is (the specification records ICU
failing 4.3%, ada 16.3% and upa 15.2% of the JSON Schema suite's `idn-hostname` cases, and
libidn2's registration check 2.2%).

### D2. The strict profile is the default

`idna::options{}` is UTS 46 with every flag on, nontransitional processing, the DNS lengths
verified, and `idna2008 = true`. UTS 46 alone accepts what IDNA2008 refuses (the code points it
marks NV8 and XV8, and every CONTEXTO violation, since UTS 46 has no CONTEXTO rules). A program
that wants UTS 46 alone writes `idna::options{.idna2008 = false}`. The default never changes to
a looser one. Alternative rejected: UTS 46 alone as the default, which would make every consumer
turn on the check it needs.

### D3. IDNA2008's property is derived, not shipped

A code point is PVALID, CONTEXTJ or CONTEXTO exactly when its UTS 46 status is `valid` or
`deviation` and it carries no NV8 or XV8 mark. The generator compares this derivation with IANA's
`idna-tables-12.0.0` on every run, for every code point IANA assigns a property, U+002E aside
(DISALLOWED in IANA, `valid` in UTS 46 because it is the separator no label holds), and stops
when they differ. No IANA table is shipped. The CONTEXTJ and CONTEXTO sets are checked against
RFC 5892 appendix A, whose rules `detail/context.hpp` hard-codes.

### D4. Two forms of every conversion, output first

A span form writes into the caller's `std::span` and allocates nothing; it is the primitive. A
pmr form returns a `std::pmr` string from a memory resource (by default
`std::pmr::get_default_resource()`) and allocates from it alone. The span forms take the output
first, as Boost.URL's encoders do, so that no call is ambiguous between the two forms. The
validators take no output and never allocate. Alternative rejected: only a `std::string` API,
which cannot honour "nothing allocates".

### D5. Errors are values, one enumerator per rule family

Every operation that can fail returns `idna::result<T>`, an alias of Boost.System's `result`, or
(for `to_unicode`, which converts as far as it can) a struct whose `error` is an `error_code`.
The 19 `idna::error` values are one per family of rules, not one per UTS 46 status code, because
IdnaTestV2 itself says an implementation "need only record that there is an error". The values
are fixed and never reused; the category is a constexpr object with the fixed name
`webcpp.idna` and the fixed identity `0x69646E6100000000`, so idna holds no mutable state. The
first error found is the one reported (D8 gives the order).

### D6. One label driver, one label's storage

A single streaming driver (`detail/process.hpp`) implements UTS 46 section 4 label by label,
with or without the mapping, into a sink that takes the ASCII form, the Unicode form or nothing.
It holds one label at a time in fixed storage of 256 code points (`detail/label_buffer.hpp`),
whatever the name's length. 256 is enough because every label the DNS lengths accept fits: an
A-label of at most 63 octets decodes to at most 59 code points, and the longest full canonical
decomposition in Unicode 17.0.0 is 4 code points (U+1FAF), which the generator measures and
checks. A label beyond the storage occurs only when a program turns the DNS lengths off or calls
`to_unicode`: the pmr forms then take storage from their resource, and the span forms report
`label_too_long` (under the DNS lengths) or `storage_exhausted`.

### D7. Punycode is bounded at 256 code points in the conversions, not in the primitive

RFC 3492's encoder passes over the input once per distinct code point and its decoder inserts
each code point among those before it, so their work grows with the square of a label. The
conversions bound a label's Punycode at the working storage's 256 code points and report
`punycode_overflow` past it, whatever `ignore_invalid_punycode` says, since a limit of the
implementation is no invalid Punycode. `punycode::encode` and `punycode::decode` are the
primitive and have no such bound: their pmr forms take input of any length in quadratic time, and
their span forms stop when the caller's span is full. The page tells a program to bound input it
does not trust.

### D8. A fixed order of errors

A name with several faults reports the first label's first fault, labels in order; then `bidi`
for the Bidi rule across the labels; then `domain_too_long`; and `output_too_small` last in
`to_ascii`, so a span too small for a name that is too long anyway reports the name. A label's
rules run in UTS 46's order (NFC, hyphens, leading mark, status, STD3, CONTEXTJ), IDNA2008's last
(NV8/XV8, then CONTEXTO). A label beyond the storage is reported while it is gathered, before its
rules, and keeps an error recorded before it. The order is pinned by tests so that lexical can
return the same reason on every target.

### D9. Generated tables and generated vectors

`tools/generate_tables.py` (Python 3.9, standard library only) writes
`include/webcpp/idna/detail/tables.hpp`, `test/generated/idna_test_v2.ipp` and
`test/generated/nfc_test.ipp` from the files `tools/unicode.json` pins by URL and SHA-256. Two
runs write the same bytes, and nothing is written unless every check passed. The tables are
`constexpr` arrays of constant data (sorted runs searched by binary search, 147,324 bytes), never
built at run time, so any number of threads may call idna at once. The vectors are C++ data so
that no test reads a file on any target. The own lane `tables` runs the generator into the build
directory and compares each output with the committed file; the ordinary lanes never run it (it
may need the network). Alternative rejected: committing the Unicode files, which would copy
Unicode's data into the repository.

### D10. The Unicode data notice lives on the page

The tables and vectors derive from Unicode's data files, whose Unicode License v3 asks for its
notice with the copies or in the documentation, while every file of webcpp is Boost Software
License 1.0. The library keeps its code under the Boost license alone, carries the notice whole in
the page's appendix "Unicode data", names it in one comment line of each generated file, and adds
no separate licence file.

### D11. Exceptions and configuration

idna raises no exception of its own, so `WEBCPP_IDNA_NO_EXCEPTIONS` changes nothing in it; it
exists so that a program can set it for every webcpp library alike. `config.hpp` includes
`<webcpp/config.hpp>` first and defines the macro when webcpp has no exceptions
(`WEBCPP_HAS_EXCEPTIONS` is not defined) or when the program defines it. A `noexcept` function
reads a result it has checked with `unsafe_value()`, since Boost 1.92's `operator*` calls
`value()`, which throws.

### D12. Unicode 17.0.0, not 18.0.0

The owner chose 17.0.0 (UTS 46 revision 35); Unicode 18.0.0 is out and the move is not made. A
move is a change of its own, and the page lists every file it changes.

## Risks / Trade-offs

- [UTS 46 is not IDNA2008] → IdnaTestV2 does not test CONTEXTO, and nothing official tests the
  combination; the strict profile rests on RFC 5892's rules case by case in idna's tests and on
  the JSON Schema suite run in lexical.
- [The derivation of IDNA2008's property rests on a measurement against IANA 12.0.0] → code
  points assigned after Unicode 12.0 have no IANA property to compare with; RFC 5892's derivation
  is stable by design, and the generator repeats the comparison on every regeneration.
- [The tables cost compile time in every translation unit that includes `uts46.hpp` or
  `validity.hpp`] → about 0.7 s and 188 KB natively for a program that calls `to_ascii`
  (measured with Apple Clang 21 at `-O2`); `punycode.hpp`, `errors.hpp` and `options.hpp` cost
  nothing of them.
- [The generator needs the network] → it is an own lane, never an ordinary one; a cache directory
  makes it offline once filled, and a moved URL fails the lane by name.
- [Stack on WebAssembly] → about 1.3 KiB of inline storage per call; the deepest call was
  measured at about 11 KiB in a debug build against 64 KiB. These are measured figures with no
  margin promised.
- [Punycode's quadratic work] → bounded in the conversions (D7), documented in the primitive.

## Migration Plan

None: a new library. While `webcpp::config` did not exist, `config.hpp` included
`<boost/config.hpp>`; once it did, the change was one line plus the alias gaining the
`/webcpp/config//config` dependency, made in commit `c7f6db7`.

## Open Questions

None for 0.1.0. Not decided, and not part of this proposal: moving to Unicode 18.0.0 (UTS 46
revision 36), and a URL host parser on top of `to_ascii`.
