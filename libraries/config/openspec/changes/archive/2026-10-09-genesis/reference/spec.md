---
title: "webcpp::config: configuration, version and the changelog rule"
tags: [webcpp, config, release, spec]
status: active
created: 2026-10-09
---

# webcpp::config

The owner's decisions of 2026-10-08 and 2026-10-09 (also recorded in
`~/.buzz/PLANS/WEBCPP_RELEASE_DESIGN.md`). Research:
`~/.buzz/RESEARCH/BOOST_RELEASE_NOTES.md`.

## 1. Purpose

webcpp's counterpart of Boost.Config: one header-only library every webcpp
library includes first, holding what is common to all of them (the target,
exceptions, the version), plus the rule and the files that keep a changelog
from now on, and the release tooling, ready but not run until the owner asks
for a release.

## 2. Decisions

1. A library of its own, repository `webcpporg/config` (created with the
   owner's go), submodule `libs/config`; `webcpp/config.hpp` includes
   `<boost/config.hpp>`.
2. webcpp has a version, as Boost does, starting at **0.1**.
3. **`WEBCPP_NO_EXCEPTIONS`** (owner, 2026-10-09, option (a)): defined by
   the developer, it disables the exceptions of every webcpp library at once,
   even in a build that has them; each library's
   `WEBCPP_<LIBRARY>_NO_EXCEPTIONS` is defined when the compiler has no
   exceptions, when `WEBCPP_NO_EXCEPTIONS` is defined, or when the developer
   defines it alone.
4. Changelogs are written from now on, in the pattern of Boost's release
   notes, by agents too; each release's changelog is the base of its "What's
   new"; a release ships a versioned `.tar.gz` on GitHub with a tag. No
   release happens until the owner asks.

## 3. Headers

```cpp
// <webcpp/config.hpp>
#include <boost/config.hpp>
#include <webcpp/version.hpp>

// The target, from the compiler's own macros, exactly one defined:
// WEBCPP_TARGET_WASIP2     (__wasip2__)
// WEBCPP_TARGET_WASIP3     (__wasip3__)
// WEBCPP_TARGET_EMSCRIPTEN (__EMSCRIPTEN__)
// WEBCPP_TARGET_NATIVE     (none of the above and not __wasm__)
// Any other wasm build (wasip1, wasm32-unknown) is an #error "unsupported
// target" (owner, 2026-10-09); a new target gets its own macro.

// Exceptions: WEBCPP_HAS_EXCEPTIONS unless BOOST_NO_EXCEPTIONS or
// WEBCPP_NO_EXCEPTIONS is defined.

// <webcpp/version.hpp>
#define WEBCPP_VERSION 100        // major * 100000 + minor * 100 + patch, as BOOST_VERSION
#define WEBCPP_LIB_VERSION "0_1"
```

Every library's `config.hpp` includes `<webcpp/config.hpp>` first and keeps
its own defines:

```cpp
#if !defined(WEBCPP_HAS_EXCEPTIONS) && !defined(WEBCPP_<LIBRARY>_NO_EXCEPTIONS)
#define WEBCPP_<LIBRARY>_NO_EXCEPTIONS
#endif
```

The `__MRDOCS__` documentation form of Task 1B (stage 4) documents each
macro in the reference. The `WEBCPP_VERSION` value is written in one place
only, `version.hpp`; the Jamroot and the release tooling read it from there.

## 4. The changelog rule (AGENTS.md)

- Each library has `doc/changelog.adoc`, rendered in its page, with a section
  per version and an "Unreleased" section on top; the superproject has
  `doc/releases/<version>.adoc`, which includes each library's section of
  that version (nothing copied twice).
- An entry is one bullet per user-visible change, a past-tense verb first
  ("Added", "Changed", "Fixed", "Removed", "Deprecated"), about ten words,
  at most ten per library and release, the most important first;
  `*Breaking change:*` inline; a deprecation names its replacement; issue and
  PR links when they exist; internal-only changes left out.
- Every change adds its line to "Unreleased" in the same commit as the
  change; agents may write them.
- Release notes sections, as Boost's: New Libraries (one line each: what it
  is, "from <authors>"), Updated Libraries (alphabetical), Removed Libraries
  (webcpp's addition: what went out), Tools and build, Compilers tested.
- The pending entries gathered so far
  (`~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md`) move into the libraries'
  "Unreleased" sections when this library lands.

## 5. Release tooling (ready, not run)

- `b2 release-check`: fails when a library's tree changed since the last tag
  and its "Unreleased" has no line, when the release notes miss a library,
  when new or removed submodules are not under New or Removed Libraries, or
  when the version disagrees anywhere.
- `b2 whats-new`: writes the skeleton of `doc/releases/<version>.whats-new.md`
  (Boost's form: at most 5 highlights, a bold category and at most 20 words
  each), which an agent fills from the release notes and the owner approves
  before the tag; it becomes the GitHub Release's body.
- `b2 dist`: a reproducible `webcpp-<version>.tar.gz` (sources, built docs,
  no dot files) and a `sha256sum` file.
- Releasing (only when the owner asks): "Unreleased" becomes the version's
  section in every library, `release-check` passes, the tag
  `webcpp-<version>` goes on the superproject and every library, and the
  GitHub Release carries the archive, its checksum and the "What's new".
  Signing is the owner's decision then (Boost signs nothing).

## 6. Tests

- `config_test`: the target macros on each lane (native, wasip2, wasip3,
  emscripten), `WEBCPP_HAS_EXCEPTIONS` with and without exceptions,
  `WEBCPP_NO_EXCEPTIONS` turning every library's macro on in a build with
  exceptions, `WEBCPP_VERSION` and `WEBCPP_LIB_VERSION` agreeing.
- Tools tests for `release-check` (a changed library without a line fails;
  a missing release-notes entry fails), `whats-new` and `dist` (reproducible
  bytes, no dot files), in a scratch git repository.

## 7. Order

After stage 4; then every library's `config.hpp` moves onto it (xactor,
xstate, pratt, wasi, trystero, and the new idna, formats and json_schema,
whose specs already say "a one-line migration"), the changelog files are
created with the pending entries, and the wasi redesign starts.

Approved by the owner on 2026-10-09.
