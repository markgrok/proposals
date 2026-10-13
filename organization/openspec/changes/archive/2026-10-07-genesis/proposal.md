## Why

A webcpp library is built, tested, linted, documented and published by one shared machine: a
superproject of submodules, one b2 build, one rulebook, one lint, one documentation toolchain, one
CI and one site. Every library proposal and every contributor, human or coding agent, relies on
that machine and has to obey its rules, yet until now it was specified only in the superproject's
`AGENTS.md`, 2,689 lines read from the top. A proposal for a library says "it follows the
organization"; this proposal states, as requirements that can be checked against the published
tree, exactly what the organization is, so that a library's genesis can stay short and an agent
can find the rule it needs.

## What Changes

- **New capability `organization`** (retroactive): the superproject (layout, submodules under
  `libs/` and `third_party/`, the rulebook and what it requires of every library), the build
  system (b2, the Jamroot, `tools/webcpp.jam`'s rules, the targets native, wasip2, wasip3 and
  emscripten, MSVC and Apple Clang on CI), the library contract (repository files, `build.jam`,
  `meta/libraries.json`, include boundaries, `config.hpp` and `WEBCPP_NO_EXCEPTIONS`, licence
  notice, Doc Comments, changelog form), the lint and its twenty-one rules, tests and own lanes,
  the documentation (MrDocs reference, AsciiDoc pages, `b2 doc`), the CI (the reusable
  `library.yml`, the superproject's `ci.yml`, the report), releases and the version
  (`webcpp::config`, release notes, `check-release`, `whats-new`, `dist`) and the commit identity
  rules.
- **New capability `site`** (retroactive): the site, `webcpporg.github.io` as the preview and
  `webcpp.org` as the release, how `b2 site` builds it from OpenUI Lang pages and every library's
  page, and how CI publishes it.
- No code changes: the requirements describe the superproject at `c53f3f9` and the site at
  `7a4c03f` as they are.

## Capabilities

### New Capabilities

- `organization`: the webcpp superproject and everything every library inherits from it (build,
  rules, lint, tests, documentation, CI, release).
- `site`: the webcpp site and its build and publication.

### Modified Capabilities

None.

## Impact

- **Kind:** the organization itself (`kind: organization`, `port_of: null`).
- **Implementation:** https://github.com/webcpporg/webcpp at `c53f3f9`, the superproject; its
  submodule `site/` (https://github.com/webcpporg/webcpporg.github.io) at `7a4c03f`;
  `reference/superproject/` and `reference/site/` are those trees.
- **Dependencies:** Boost 1.92, b2 5.5.3, wasi-sdk 34, wasmtime 47.0.3, wit-bindgen 0.62.0,
  Emscripten 6.0.11, Node 26.7.0, MrDocs 2026.9.29, Python 3.9+ (the prerequisites requirement).
- **Status:** implemented; the version it carries, 0.1.0, is in preparation and not yet tagged
  (`reference/STATUS.md`).
- **Other proposals:** every library's genesis relies on `organization`; the change
  `site-increments-4-5` modifies `site`.
