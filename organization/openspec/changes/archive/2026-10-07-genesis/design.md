## Context

webcpp began on 2026-10-07 as a superproject of its own, and by 2026-10-13 it held nine libraries, a build, a lint, a documentation toolchain, a CI and a
site, all by the same maintainer (`R`) and the coding agents working for him, in 261 commits from
`bb8533b` (2026-10-07) to `c53f3f9` (2026-10-13). The rulebook `AGENTS.md` was written alongside,
as the rules were found: its chapters 1 to 12 describe the tree as it is, and chapter 13 holds the
roadmap. The infrastructure for the first libraries was planned in `reference/shared-infra-plan.md`
(the check of generated files, `third_party/`, the compiled Boost libraries, test data on WASI),
the release and version design is `reference/release-design.md`, and the site was specified in
`reference/site-spec.md` and planned in `reference/site-plan.md` (its increment 1 is built).

This document records, retroactively, the decisions the organization is built on. It adds no
decision of its own.

## Goals / Non-Goals

**Goals:**

- State, as checkable requirements, how webcpp is organized and how a library is built inside it,
  so that a library's proposal states only what is specific to the library.
- Keep one rulebook, one build and one lint for every library, so that a rule is enforced by a tool
  (the lint, a Jam rule, the report) rather than by memory.
- Make the evidence a green run, from scratch, on every target, with the CI running exactly the
  commands the rulebook documents.

**Non-Goals:**

- Anything the roadmap holds and the tree does not: the three levels of CI, the review of the older
  libraries against the allocator, exception and fixed-width rules, webcpp::asio, webcpp::beast,
  webcpp::webrtc, webcpp::crypto, QuickJS, libp2p and HTTP fetch, external libraries built from
  `third_party/`, bundled toolchains, and the site's increments 2 to 8 (the change
  `site-increments-4-5` takes increments 4 and 5).
- The libraries' own requirements (each library has its own proposal) and the content of the
  site's later pages.
- Publishing, tagging and the release channel: nothing is released until the owner asks.

## Decisions

### D1. Boost's model: a superproject, libraries as submodules, one Jamroot
The superproject is the equivalent of `boostorg/boost` and `webcpporg` of `boostorg`; each library
is a repository of its own, a submodule under `libs/<name>` with a relative URL (`../<name>.git`),
and has no Jamroot of its own: the superproject's Jamroot is every library's build configuration.
Alternative rejected: independent repositories each with a build, which would let the rules drift
and make a cross-library change two unrelated commits with no joint CI.

### D2. b2 builds, tests, documents and serves everything
Everything is a b2 target: tests, examples, docs, install, the site, the release. Python scripts
(`tools/`) and Node scripts (the site) are procedures that a target runs, never an alternative
entry point. Alternative rejected: CMake or a Makefile front end, which Boost's own libraries do
not need and which would hold a second copy of the build knowledge.

### D3. The rulebook is AGENTS.md, and the lint enforces what a pattern can
`AGENTS.md` is written for coding agents and humans alike; each rule that a tool can check is
checked by `tools/lint/lint.sh` (twenty-one rules, including webcpp's own: the licence notice, the
banned word, the em dash, JSON literals, the world rule, include boundaries, the bare throw, the
changelog form), by a Jam rule that refuses a wrong declaration, or by the report. A rule that
needs judgment (a changelog line "says what a user sees") is the reviewer's, and says so.

### D4. Every target builds with exceptions; whether a program uses them is its builder's choice
Exceptions and RTTI are never imposed. Each library has `WEBCPP_<NAME>_NO_EXCEPTIONS`, raises
through `boost::throw_exception` alone, works with `-fno-exceptions`, and the lint analyses every
program and aggregate a second time without exceptions. On WASI the Jamroot encodes exceptions as
standard WebAssembly exceptions (exnref) because wasi-sdk 34's legacy encoding is refused by
wasmtime 47.

### D5. Compiled Boost libraries are built from Boost's sources, for every target
Boost.URL, Boost.Charconv, Boost.Locale and Boost.Cobalt are built by webcpp from Boost's own
source archive (`tools/boost/Jamfile`), as Boost.JSON's definitions are, so that native, wasip2,
wasip3 and emscripten use the same sources with the same macros; an installed Boost cannot be
relied on (Homebrew's has no `libs/url/src`). The sources' version must equal the configured
Boost's.

### D6. A port's evidence is the original itself
A port is proven by the pinned original running the same cases and the twins of the examples; an
expected result is written by `update-expected` only. A library of webcpp's own is proven by tests
and by the oracles that fit it (the official JSON-Schema-Test-Suite, V8). Differences from an
original are recorded decisions with an example and a twin each.

### D7. Errors as values, no hidden state, determinism, allocation under the user's control
A fallible operation returns `boost::system::result<T>` with a category `webcpp.<name>`; there is
no global mutable state; no clock, file, network, process, thread, environment or entropy unless it
is the library's purpose and the line says why; shareable data structures take an `Allocator`, the
rest a `std::pmr::memory_resource`. Libraries written before the rule are brought to it by a review
on the roadmap.

### D8. Documentation is generated and checked, never typed twice
The reference comes from Doc Comments through MrDocs, strict; the page from AsciiDoc through
Asciidoctor.js; every listing is a compiled example and every output a `.expected` file; counts are
computed; links between pages are checked. The page's build configuration is included from the
tracked files, never copied.

### D9. The CI is the commands, plus a plan that reads the declarations
The CI never lists a library's targets by hand: `b2 declared-targets` and `b2 declared-lanes`
produce the matrix. One reusable workflow serves every library and the superproject; lanes are
isolated by build directory; containers with no network prove that tests reach nothing beyond the
machine; external libraries are installed by pinned actions until webcpp builds them.

### D10. One version, in config; a changelog written with every change
`WEBCPP_VERSION` is written once, in `libs/config`; every user-visible change adds a changelog line
in the same commit (agents write them, replacing the global never-edit rule for webcpp); the
release is a procedure with three targets (`check-release`, `whats-new`, `dist`) and a reproducible
archive; nothing is released until the owner asks.

### D11. The site is a repository of its own, built by the superproject's command
The site is webcpp built with webcpp: a JavaScript application of OpenUI Lang pages rendered to
static HTML, in a submodule, built only by `b2 site` (which calls the site's npm scripts through one
wrapper), swapped into `bin/site` in one rename, published to `gh-pages` by the superproject's CI;
the preview is never indexed.

### D12. Commit identity is `R`, with no trailer
Maintainers and their agents commit as `R <github.overboard312@passmail.net>` with no trailer; the
lint refuses the banned word in any commit; outward actions (push, merge, create a repository,
change a setting) are the owner's.

## Risks / Trade-offs

- **The rulebook is long.** 2,689 lines; mitigated by this specification's grouping by area and by
  the lint, which enforces the rules a tool can.
- **Windows is the least measured target.** Boost.Locale and Boost.Cobalt's Windows behaviour was
  not measured when written; the first MSVC run that builds a library is its measurement.
  The path handling of Jam files on Windows was only fixed with the last commits (`path.exists`
  given native paths).
- **External libraries are installed by hand or by CI actions** until webcpp builds them, which
  makes trystero's lanes depend on three actions and a container image.
- **Some prose in `AGENTS.md` lags the tree** (its table of libraries names `libs/formats` for
  lexical, which is `libs/lexical` in `.gitmodules`); the tools' tests pin behavior, not the prose.

## Migration Plan

None: this is a retroactive specification. Archiving it writes `openspec/specs/organization/spec.md`
and `openspec/specs/site/spec.md`. The version 0.1.0 is released by the procedure of the
requirement "Release: The procedure" when the owner asks.

## Open Questions

- **O1.** When does the first release (0.1.0) get its tag, and does the owner sign it? The notes are
  in preparation, `check-release` has not been run against a final tree.
- **O2.** Boost.Locale on Windows (MSVC): unmeasured; the first CI run that builds it decides.
- **O3.** The shape of the three CI levels (fast, nightly, release) and of the review of the older
  libraries: roadmap, not decided.
- **O4.** How asynchronous libraries meet Boost.Asio and Boost.Cobalt on xactor: not designed.
