---
title: "webcpp releases, version and config: design (draft, decisions so far)"
tags: [webcpp, release, design]
status: draft
created: 2026-10-08
---

# webcpp releases, version and config (draft)

The owner's decisions of 2026-10-08, recorded as said.

1. **`webcpp/config.hpp` exists**, in the philosophy of `boost/config.hpp`,
   and includes `<boost/config.hpp>`. It is **a library of its own, with a
   repository of its own**, as Boost.Config is. Every library's
   `config.hpp` includes it.
2. **webcpp has a version, as Boost does**, starting now at **0.1**.
3. **Every release has a "What's new"** with a changelog of everything that
   came in and went out, in the pattern of Boost's release notes.
4. **The changelog rule** goes into the superproject's AGENTS.md: how
   changelogs are written, following Boost's (features summarised one per
   line, a list of fixed bugs, and so on). Agents may write them; for
   webcpp this replaces the global rule of never editing a changelog. The
   changelog is the base of the "What's new" post of each release.
5. **Each release produces a `.tar.gz` with the version**, distributed on
   GitHub with a tag.

Research: `~/.buzz/RESEARCH/BOOST_RELEASE_NOTES.md` (in progress).
6. **No release yet (owner, 2026-10-08).** Nothing is released until the
   owner asks. Changelogs are written from now on, with every change, so
   they are ready then: each task's report carries a "Changelog" section in
   Boost's style per library; the controller gathers them in
   `~/.buzz/PLANS/WEBCPP_CHANGELOG_PENDING.md` until each library has its
   `doc/changelog.adoc` with an "Unreleased" section (this design), into
   which they move; on a release, "Unreleased" becomes the version's
   section. Order: stage 4, then this design (config library, version 0.1
   in `version.hpp`, the changelog rule and files, the release tooling
   ready but not run), then the wasi redesign, then the rest.
