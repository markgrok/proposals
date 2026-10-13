## Why

A web program that accepts a host name from outside has to know whether an internationalized
domain name is valid, and what its ASCII form is, before it looks the name up, stores it or
compares it. The answer is not one standard's: UTS 46 says how to map and convert what a person
typed, IDNA2008 (RFC 5890 to 5893) says which labels are valid, and the two disagree (UTS 46
accepts `¡` and has no rules for CONTEXTO code points, which IDNA2008 refuses). C++ had no small,
dependency-free library that implements both, allocates nothing on its validation path, reports
every failure as a value, runs on WebAssembly, and is proven by Unicode's own conformance file.
webcpp::lexical needs one for the JSON Schema formats `hostname`, `idn-hostname`, `email` and
`idn-email`, which ask for IDNA2008's strictness; the libraries measured against the JSON Schema
suite in the library's specification each accepted labels IDNA2008 refuses.

## What Changes

- **New library `webcpp::idna`**, webcpp's own (not a port), header-only C++20: Punycode
  (RFC 3492) `encode` and `decode`; UTS 46's `to_ascii` and `to_unicode` (sections 4.2 and 4.3,
  revision 35); IDNA2008 validity of a label or a domain name as given (`check_label`,
  `check_domain`, `is_valid_label`, `is_valid_domain`) with the contextual rules of RFC 5892
  appendix A and the Bidi rule of RFC 5893; all over Unicode 17.0.0 tables generated into the
  headers.
- The default `idna::options` is the strict profile: UTS 46 with every check on,
  nontransitional, the DNS lengths verified, and IDNA2008's validity on top (`idna2008 = true`).
  Each check is a member that a program turns off by name.
- Every operation that can fail returns `idna::result<T>` (Boost.System's `result`) whose error is
  one of 19 fixed `idna::error` values of the category `webcpp.idna`; idna throws nothing of its
  own.
- Every conversion has a span form, which writes into the caller's buffer and allocates nothing,
  and a pmr form, which returns a string from a `std::pmr::memory_resource`. The validators
  allocate nothing.
- A Python generator, `tools/generate_tables.py`, writes the tables and the conformance vectors
  from Unicode files pinned by URL and SHA-256, after checking what the library assumes of the
  data; the own lane `tables` proves the committed files are its output.
- Proven by all 6,391 lines of IdnaTestV2.txt (toUnicode, toAsciiN, toAsciiT) and all 20,034
  lines of NormalizationTest.txt, with an empty list of known failures, by RFC 3492's samples,
  by tests of every rule of RFC 5892 appendix A and RFC 5893 section 2, and by a test that counts
  every call of the global `operator new`, on native, wasip2, wasip3 and Emscripten.

## Capabilities

### New Capabilities

- `idna`: webcpp::idna 0.1.0, internationalized domain names (Punycode, UTS 46 ToASCII and
  ToUnicode, IDNA2008 validity) for C++20.

### Modified Capabilities

None.

## Impact

- **Kind:** a library of webcpp's own (`"port-of": null`).
- **Implementation:** https://github.com/webcpporg/idna at `e2452e5`, the submodule `libs/idna`
  of the superproject; `reference/source/` is exactly `git archive e2452e5` of that tree.
- **Dependencies:** Boost's headers (Config, System, Core, Assert; Boost 1.92 or newer) and
  webcpp::config; nothing is built or linked for idna. Build-time only: Python 3.9 or newer
  (standard library) for the generator, which downloads Unicode's files when its cache lacks
  them. The data derived from Unicode's files is under the Unicode License v3; idna's code is
  under the Boost Software License alone.
- **Status:** implemented, release 0.1.0 (`reference/STATUS.md`).
- **Other proposals:** the `lexical` capability depends on idna's public interface
  (`errors.hpp`, `options.hpp`, `punycode.hpp`, `validity.hpp`, `uts46.hpp`): a change to one is
  announced to it.
