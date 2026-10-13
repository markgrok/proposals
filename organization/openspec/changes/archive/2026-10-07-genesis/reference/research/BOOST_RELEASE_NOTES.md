---
title: "Boost release notes, changelogs and packaging"
tags: [webcpp, release, research]
status: active
created: 2026-10-08
---

# Boost release notes, changelogs and packaging

How Boost writes its release notes and per-library changelogs, how it builds,
tags and announces a release, and a concrete proposal for webcpp
(`webcpporg`, starting at 0.1). Every claim below was read on 2026-10-08 from
the sources cited next to it; the raw material was downloaded to
`/tmp/bw/` (disposable).

Context: `~/.buzz/PLANS/WEBCPP_RELEASE_DESIGN.md` (the owner's decisions of
2026-10-08) and `~/.buzz/PLANS/WEBCPP_DESIGN.md`.

## 0. Where Boost's release notes live now

- Up to 1.89, the release notes were QuickBook files in the old website
  repository: `feed/history/boost_1_NN_0.qbk` in
  https://github.com/boostorg/website/tree/master/feed/history. The last
  real commit there is from 2025-12 (System notes for 1.89); `boost_1_90_0.qbk`
  is only a 63-line stub.
- From 1.90 on, they are AsciiDoc files in
  https://github.com/boostorg/website-v2-docs/tree/develop/release-notes:
  `boost_1_90_0.adoc`, `boost_1_91_0.adoc`, `boost_1_92_0.adoc`, a stub
  `boost_1_93_0.adoc` (already one PolyCollection entry), and `template.adoc`.
  The new site (https://github.com/boostorg/website-v2) imports them from that
  repository's `master` branch (`versions/releases.py`,
  `release-notes/master/<file>.adoc`), and `publish_release.py` refuses to
  continue while website-v2-docs `master` lags `develop`
  (https://github.com/boostorg/release-tools/blob/master/publish_release.py).
- Rendered at https://www.boost.org/releases/1.92.0/ (old URLs such as
  `/users/history/version_1_92_0.html` redirect there).
- Releases existing as of 2026-10-08: 1.88.0 (2025-04-10), 1.89.0
  (2025-08-14), 1.90.0 (2025-12-10), 1.91.0 (April 2026, re-released as
  1.91.0-1 on 2026-04-23), 1.92.0 (2026-08-12). 1.93 is in progress.
  Source: `gh release list -R boostorg/boost`.

## 1. The release notes of 1.88 to 1.92

### 1.1 The file and its sections

`template.adoc` (https://github.com/boostorg/website-v2-docs/blob/develop/release-notes/template.adoc)
is the skeleton every release starts from:

```
== Version X.XX.XX
boost_at:/doc/libs/X_XX_X/[Documentation]
// Please keep the list of libraries sorted in lexicographical order.
== New Libraries
== Updated Libraries
== Compilers Tested
== Acknowledgements
```

Sections that actually appeared (headings read with `grep '^=='` and
`grep '^\[section'`):

| Release | Sections, in order |
| --- | --- |
| 1.82 | Known Issues, Notice of Dropping C++03 Support, New Libraries, Updated Libraries, Updated Tools, Compilers Tested, Acknowledgements |
| 1.85 | Known Issues, New Libraries, Updated Libraries, Updated Tools, Compilers Tested, Acknowledgements |
| 1.88 | New Libraries (Hash2, MQTT5), Updated Libraries (20), Compilers Tested, Acknowledgements |
| 1.89 | New Libraries (Bloom), Updated Libraries (29), Updated Tools (B2), Compilers Tested, Acknowledgements |
| 1.90 | New Libraries (OpenMethod), Updated Libraries (38), Compilers Tested, Acknowledgements |
| 1.91 | General Notes, New Libraries (Decimal), Updated Libraries (30), Compilers Tested, Acknowledgements |
| 1.92 | General Notes, New Libraries (none), Updated Libraries (35), Updated Tools (B2), Compilers Tested, Acknowledgements |

So the fixed core is **New Libraries / Updated Libraries / Compilers Tested /
Acknowledgements**, with optional sections placed around it:

- **General Notes** (first): collection-wide changes. 1.91: StaticAssert
  merged into Config. 1.92: `b2 install` puts `.dll` files in `bin`, and the
  installed CMake config accepts header-only components. Written as bullets
  with `+` continuation paragraphs.
- **Known Issues** (first, older releases): "patches from library authors
  which were found too late to be fixed in the release", each a bullet with
  the issue and a link to a patch under https://www.boost.org/patches/
  (for example `patches/1_85_0/0001-container-fix-flat_map.patch`). For 1.91
  the patch `patches/1_91_0/0001-optional-copy-construct.patch` was added to
  the old website repository on 2026-05-14 without a Known Issues section.
- **Notice of ...** (one-off notices such as dropping C++03, 1.82).
- **Updated Tools**: B2, one bullet ("Includes release of B2 version 5.5.3").
- **Compilers Tested**: a static list (Linux Clang/GCC per standard, OS X
  Apple Clang, Windows Visual C++), the same text release after release.
- **Acknowledgements**: one sentence, "Marshall Clow, Glen Fernandes and Ion
  Gaztañaga managed this release."

There is no "Removed Libraries" or "Platform" section in these five releases,
and no "Patches" section; patches are linked from Known Issues.

### 1.2 How a new library is announced

One top-level bullet with the library name linked to `/libs/<name>/`, and
**one sub-bullet of one sentence: what it is, then "from <authors>."**

- 1.89 (qbk): Bloom, "Bloom filters, from Joaquín M López Muñoz."
- 1.88 (qbk): Hash2, "An extensible hashing framework, from Peter Dimov and
  Christian Mazakas."
- 1.90: OpenMethod, "Open-(multi-)methods in C++17 and above, from
  Jean-Louis Leroy."
- 1.91: Decimal, one sentence: an implementation of IEEE754 decimal floating
  point, from Matt Borland and Christopher Kormanyos.

No feature list, no history: the library's own documentation carries that.
The same one line is reused verbatim in the mailing-list announcement
(section 4).

### 1.3 How an updated library's entry is written

AsciiDoc form (1.90 onward), from `template.adoc`:

```
* boost_phrase:library[Interprocess,/libs/interprocess/]:
** Added anonymous shared memory for UNIX systems.
** Conform to `std::pointer_traits` requirements (boost_gh:pr[interprocess,32]).
** Fixed `named_condition_any` fails to notify (boost_gh:issue[interprocess,62]).
```

QuickBook form (up to 1.89): `* [phrase library..[@/libs/x/ X]:]` then
two-space-indented `  * ...` items, links `[github x 62]` and
`[github_pr x 32]`.

Rules observed:

- **Libraries sorted lexicographically**, one top-level bullet each, the name
  linked to the library. Only libraries that changed are listed.
- **One second-level bullet per change, one sentence**, usually starting with
  a past-tense verb: Added, Changed, Fixed, Removed, Improved, Deprecated,
  Migrated, Switched. Across 1.88 to 1.92 the counts of bullets starting
  "Fix" were 21, 30, 38, 20, 34 and "Add" 17, 28, 31, 36, 24. Some
  maintainers (Beast) write present-tense behaviour statements instead:
  "`websocket::stream` accepts close code 1014 (bad gateway)."
- **Fixed bugs**: usually one "Fixed ..." bullet each. A library with many
  uses a nested group: Container 1.92 has "Fixed bugs/issues:" with third-level
  bullets, each "description (boost_gh:issue[container,334])". URL 1.91 groups
  under "Features:", "Performance:", "Fixes:", "Documentation:" with
  third-level bullets. Grouping is the exception, not the rule.
- **Issue and PR links**: macros from
  https://github.com/cppalliance/asciidoctor-boost.
  `boost_gh:issue[geometry, 1231]` renders as `#1231` linking to
  `https://github.com/boostorg/geometry/issues/1231`;
  `boost_gh:pr[charconv, 321]` renders as `PR#321` linking to `.../pull/321`;
  `boost_at:/libs/function[Function]` is a site-relative link. The link sits
  in parentheses at the end of the sentence. Full URLs also appear (12 to 24
  per release, mostly contributors' GitHub profiles and other repositories'
  issues, e.g. MSM `https://github.com/boostorg/msm/issues/180[#180]`). Usage
  of the macros per release: 1.90 27 issue + 26 PR, 1.91 11 + 38,
  1.92 13 + 56. Most bullets carry no link at all.
- **Breaking changes**: no dedicated section in the history file; marked
  inline at the start of the bullet: `*Breaking change:*` (MultiIndex 1.91,
  twice), `*Breaking change for Cygwin users.*` (Filesystem),
  `**Breaking changes (`backmp11`)**:` (MSM). 1.91 has 6 mentions, the others
  0 or 1.
- **Deprecations**: a plain bullet saying what is deprecated and that it
  "will be removed in a future release", often with the replacement
  ("Use the non-templated versions instead", Filesystem 1.92). 4 to 10 per
  release.
- **Thanks**: inline, "Many thanks to <linked name> for the PR!" or
  "Contributed by <name>." (4 to 10 per release).
- **The long tail goes to the library's own changelog**: Asio ends its entry
  with "Consult the boost_at:/doc/libs/1_92_0/doc/html/boost_asio/history.html[Revision History]
  for further details."

### 1.4 How terse it is (measured)

Script `/tmp/bw/stats.py` over the Updated Libraries section of each file
(second-level bullets per library; physical lines include the library line):

| Release | Libraries | Bullets per library, median / mean / max | Libraries with 3 bullets or fewer | Lines per entry, median / max | Biggest entries |
| --- | --- | --- | --- | --- | --- |
| 1.88 | 20 | 2.5 / 4.0 / 12 | 12 of 20 | 6.5 / 35 | Asio 12, DLL 11, MySQL 10 |
| 1.89 | 29 | 2 / 2.8 / 13 | 21 of 29 | 3 / 25 | Asio 13, Atomic 7, Math 7 |
| 1.90 | 38 | 2 / 3.8 / 26 | 29 of 38 | 4 / 55 | Asio 26, Parser 11, DynamicBitset 10 |
| 1.91 | 30 | 2 / 3.5 / 14 | 20 of 30 | 3 / 39 | Asio 14, Math 10, Redis 8 |
| 1.92 | 35 | 2 / 3.2 / 24 | 26 of 35 | 4 / 24 | Asio 24, Beast 12, Graph 5, URL 5 |

Words per bullet (1.90, 1.91, 1.92): median 9, 10 and 11 words; 90th
percentile 21 to 27; maximum 62 to 84 (MultiIndex's breaking-change
paragraphs). About three quarters of bullets are a single sentence.

**Typical entry: two bullets of about ten words each.** Three quarters of the
libraries have three bullets or fewer; Asio is always the outlier.

The contributor guide states the target explicitly
(https://github.com/boostorg/website-v2-docs/blob/develop/contributor-guide/modules/ROOT/pages/release-notes.adoc,
"Best Practices"): the first three bullets or fewer for the most impactful
changes, then at most seven (five is better) concise changes, a single bullet
for all documentation work, and "keep the number of bullet points to ten or
less".

### 1.5 Representative entries (short quotes)

- JSON 1.90: "Switched to a faster hashing algorithm."
- ContainerHash 1.92: "Hashing a valueless `std::variant` instance no longer throws."
- Unordered 1.91: "Fixed the returned value of range insertion in concurrent
  containers (PR#344)."
- Beast 1.92: "`websocket::stream` accepts close code 1014 (bad gateway)."
- MultiIndex 1.91 (paraphrased): a bullet opening with `*Breaking change:*`,
  stating that `composite_key` became variadic, what users see differ
  (`std::tuple` instead of `boost::tuple`) and that the change should be
  transparent otherwise.

## 2. Per-library changelogs

Boost has **two artifacts** and says so: "Two versions of your release notes
need to be prepared: one for your library's own documentation, one for a
Boost history that covers all libraries added or updated in a release", the
second "a succinct version of your completed release notes" with a specified
format; the first has "no strict format" (contributor guide, link above). Its
checklist for the library version: header with version and date, summary,
breaking changes first, API changes, new features, bug fixes with issue
links, deprecations, performance, documentation, acknowledgements, known
issues, upgrade notes.

What six libraries actually do (`develop`, read with `gh api`):

| Library | File | Format | Heading per release | Relation to the Boost notes |
| --- | --- | --- | --- | --- |
| JSON | `CHANGELOG.adoc` (root, included as the docs' Release Notes) | AsciiDoc; groups `.Fixes`, `.Improvements`, `.API Changes`; each bullet starts with the issue link `{issue}1160[#1160]` (attribute `:issue: https://github.com/boostorg/json/issues/`) | `== Boost 1.92.0` | Boost notes are a shorter rewrite (1.90: two bullets vs. grouped list). Its 1.91 section repeats the `.Fixes` block twice, an unchecked copy error |
| Unordered | `doc/modules/ROOT/pages/changes.adoc` (Antora page "Change Log") | AsciiDoc bullets; links `{github-pr-url}/348[PR#348^]` | `== Release 1.92.0` (major ones: `== Release 1.87.0 - Major update`) | Boost notes are the same bullets, copied |
| Mp11 | `doc/mp11/changelog.adoc` ("Revision History") | AsciiDoc, Markdown-style `##` headings, one bullet each | `## Changes in 1.92.0` | Copied |
| Hash2 | `doc/hash2/changes.adoc` | AsciiDoc | per release | Copied |
| Beast | `doc/qbk/release_notes.qbk` (per Boost release) and `CHANGELOG.md` (per internal "Version 362" commit-level log) | QuickBook; groups `[*API Changes]` with `['Actions Required]:`, `[*Fixes]`, `[*Improvements]`; every bullet starts `[issue 3094]` | `[heading Boost 1.92]` | Boost notes are the release_notes bullets without the issue links and groups |
| Asio | `doc/history.qbk` ("Revision History") | QuickBook bullets, several sentences each | `[heading Asio 1.38.2 / Boost 1.92 beta]` (Asio's own version) | Boost notes are a one-sentence-per-bullet summary plus a link to the full history |
| URL | none in tree (`doc/modules/ROOT/nav.adoc` has no changelog page) | | | Boost notes are the only record |
| Decimal | none in tree; GitHub Releases with GitHub's auto-generated "What's Changed" (PR titles "by @user in <url>", dependabot included), tags `v6.0.1` and `boost-1.92.0` | | | Boost notes are a separate two-bullet summary |

Who writes the Boost notes: **each maintainer, by pull request to
boostorg/website-v2-docs `develop`**, one PR per library or per maintainer.
History of `release-notes/boost_1_92_0.adoc` (`gh api .../commits?path=`):
the stub was added 2026-04-24, right after 1.91, by Joaquín M López Muñoz;
then about thirty commits by Peter Dimov (General Notes, Assert, Mp11,
System, Hash2), Ion Gaztañaga (Container, Interprocess), Alan de Freitas
(URL), Mohammad Nejati (Beast), Christopher Kohlhoff (Asio, on 2026-08-10, two
days before the release), Matt Borland (Charconv, Decimal, Math,
Multiprecision), René Rivera (B2), and others, plus typo fixes by third
parties. Release managers do not write library entries.

## 3. Release process and packaging

### 3.1 Calendar and branches

Three releases a year, second week of April, August and December, built from
`master` of boostorg/boost; master closes in steps from 7 weeks before, a
beta about 4 weeks before, release candidates 4 days before, and point
releases when a release candidate problem warrants it
(https://github.com/boostorg/website-v2-docs/blob/develop/contributor-guide/modules/ROOT/pages/release-process.adoc).

### 3.2 Archives

Two channels, two naming schemes:

1. **archives.boost.io** (the official download, linked from the release
   page and the announcement), built by
   https://github.com/boostorg/release-tools/blob/master/ci_boost_release.py
   and published by `publish_release.py`:
   - `https://archives.boost.io/release/1.92.0/source/`:
     `boost_1_92_0.tar.gz`, `.tar.bz2` (LF line endings), `.zip`, `.7z`
     (CRLF), each with a `.json` sidecar, plus the release candidates
     `boost_1_92_0_rc1.*`. Also `source-nodocs/` (same names, without built
     docs) and `binaries/` (Windows installers
     `boost_1_92_0-msvc-14.3-64.exe`, ...).
   - Betas: `https://archives.boost.io/beta/1.92.0.beta1/source/boost_1_92_0_b1.tar.gz`
     and `boost_1_92_0_b1_rc1.*`.
   - The `.json` sidecar (read from `boost_1_92_0.tar.gz.json`):
     `{"commit": "afdfa325...", "file": "boost_1_92_0.tar.gz", "created": "2026-08-05T17:03:31Z", "sha256": "c4a3b310..."}`.
     `publish_release.py` recomputes each SHA-256 and fails on a mismatch.
   - **No signatures**: no `.asc` or `.sig` file, and no signing step in
     either script. Integrity rests on the SHA-256 in the `.json`, on the
     release page ("SHA256 Hash" column) and in the beta announcements.
2. **GitHub Releases of boostorg/boost**
   (https://github.com/boostorg/boost/releases/tag/boost-1.92.0), built by
   https://github.com/boostorg/boost/blob/master/.github/workflows/release-cmake.yml
   on every pushed tag `boost-*`:
   - `boost-1.92.0-cmake.{tar.gz,tar.xz,zip,7z}`: the checkout with
     submodules, every `.git` removed, nothing else changed.
   - `boost-1.92.0-b2-nodocs.{tar.gz,tar.xz,zip,7z}`: the same, with
     `libs/*/include/boost` copied to a top-level `boost/`, `libs/*/include`
     and `libs/*/doc` removed, and `CMakeLists.txt` removed.
   - Each with a checksum file `<archive>.txt`, written by
     `sha256sum <archive> > <archive>.txt`, e.g.
     `e2a814b3...  boost-1.92.0-b2-nodocs.tar.gz`.
   - The GitHub release body is empty. Release name = tag name.
   - 1.91.0 has a second release, `boost-1.91.0-1`, whose only difference
     from `boost-1.91.0` is a fix to `release-cmake.yml`
     (`gh api repos/boostorg/boost/compare/boost-1.91.0...boost-1.91.0-1`):
     a re-run of the packaging, not a code change.

### 3.3 What the archive contains

Listing of `boost_1_92_0.tar.gz` (93,850 entries) and
`boost-1.92.0-b2-nodocs.tar.gz` (53,648):

- Top directory named after the archive: `boost_1_92_0/` and
  `boost-1.92.0/`.
- Root: `boost/` (all headers, flattened: 17,749 entries, and no
  `libs/*/include` left), `libs/` (each library's `src`, `test`, `example`,
  `doc`, `build`, `meta`, `CMakeLists.txt`, `build.jam`, its changelog file,
  e.g. `libs/json/CHANGELOG.adoc`), `tools/` (B2, BoostBook, Quickbook,
  ...), `doc/` with **built HTML** (`doc/html`, 10,836 files, and
  `doc/antora`, 1,458), `more/`, `status/`, `Jamroot`, `boostcpp.jam`,
  `boost-build.jam`, `bootstrap.sh`/`.bat`, `index.html`, `INSTALL`,
  `README.md`, `LICENSE_1_0.txt`.
- **Dot files**: the archives.boost.io archive has none; `MakeBoostDistro.py`
  ignores `[.]*`, `.gitattributes`, `.gitignore`, `.gitmodules`,
  `.travis.yml`, `appveyor.yml`, `circle.yml`, and `ci_boost_release.py`
  excludes its own two scripts. The GitHub archives keep every dot file
  except `.git` (1,021 entries matching `.github`, `.drone`, `.gitignore`,
  `.travis` in the nodocs tarball; `libs/json/.github/`, `.drone.star`,
  `.codecov.yml` ...).

### 3.4 Tags

- `publish_release.py` tags the release with `git tag boost-1.92.0` in the
  superproject and `git submodule foreach 'git tag boost-1.92.0'`, then
  pushes the tag in every repository. **Every library repository carries the
  same tag**: boostorg/json and boostorg/url have `boost-1.92.0`,
  `boost-1.92.0.beta1`, `boost-1.91.0`, ...
- The tags are **lightweight and unsigned**: `git/ref/tags/boost-1.92.0`
  points straight at a commit (`afdfa325...`, the commit in the `.json`).
- Betas are tagged `boost-1.92.0.beta1`. Release candidates are **not**
  tagged ("This is a release candidate. Not tagging.").
- A re-packaging gets a suffix: `boost-1.91.0-1`.
- Libraries may keep their own versions besides: Asio 1.38.2, Beast's
  "Version 362", Decimal `v6.0.1` (and Decimal also has GitHub releases named
  "Boost.Decimal 1.92" on tag `boost-1.92.0`).

### 3.5 BOOST_VERSION and BOOST_LIB_VERSION

`include/boost/version.hpp` lives in **Boost.Config**
(https://github.com/boostorg/config/blob/develop/include/boost/version.hpp):

```
//  BOOST_VERSION % 100 is the patch level
//  BOOST_VERSION / 100 % 1000 is the minor version
//  BOOST_VERSION / 100000 is the major version
#define BOOST_VERSION 109300
#define BOOST_LIB_VERSION "1_93"
```

`BOOST_LIB_VERSION` is "x_y[_z]", with `_z` only when the patch level is not
0; it selects the library name for auto-linking. The header warns it is the
only one guaranteed to change every release.

Who bumps it, and when: a release manager, **on `develop` right after the
release, to the next version**. Commits: "Bump version to 1.93.0" by Ion
Gaztañaga on 2026-08-12 (the day 1.92.0 shipped), "Update Boost version to
1.92" by Marshall Clow on 2026-04-22, "Update BOOST_VERSION and
BOOST_LIB_VERSION to 1.91" on 2025-12-10. The same version is repeated by
hand in the superproject: `constant BOOST_VERSION : 1.93.0 ;` in `Jamroot`
(bumped 2026-08-12 too) and `project(Boost VERSION 1.93.0 ...)` in
`CMakeLists.txt`. Nothing checks the three agree.

## 4. Boost's "What's new" and the announcements

Three outputs are derived from the release notes, none written by the
maintainers:

1. **Mailing-list announcement** on boost-announce
   (https://lists.boost.org/archives/list/boost-announce@lists.boost.org/,
   subject "[release] Boost 1.92.0"), signed by the release team. A fixed
   template of about fifteen lines: "Release 1.92.0 of the Boost C++
   Libraries is now available.", two sentences of boilerplate about Boost,
   **the New Libraries list copied one line each** (1.91: "Decimal: An
   implementation of IEEE754 Decimal Floating Point Numbers, from ..."),
   "as well as numerous enhancements and bug fixes for existing libraries",
   links to the release page, the archive directory and getting started,
   thanks. The beta announcement adds the SHA-256 of each archive and asks
   for testing. It never lists updated libraries.
2. **"What's new in 1.92.0" card** at the top of
   https://www.boost.org/releases/1.92.0/, above the download table and the
   full release notes. It is **generated by an LLM from the release notes**
   (https://github.com/boostorg/website-v2/blob/develop/versions/tasks.py,
   `WHATS_NEW_SYSTEM_PROMPT`, model from the setting `WHATS_NEW_MODEL`),
   stored as `Version.whats_new` and **shown only after an admin approves
   it** (`whats_new_approved`, admin actions "Approve What's New (publish)"
   and "Regenerate What's New"). Rules of the prompt: a Markdown list of at
   most 5 bullets, each a bold category label and one sentence of at most 20
   words, no emoji, identifiers in backticks, no library names (the
   Dependencies bullet may name up to two), omit a category without content,
   in this order: New libraries, Performance improvements, Dependencies,
   Security & reliability, Developer experience. Dependency counts are
   precomputed from imported data and passed in. The 1.92 card has four:
   Performance improvements, Dependencies, Security & reliability, Developer
   experience.
3. **Release Report**, a PDF linked from the release page
   (`release-report-boost-1-92-0.pdf`), generated by the site
   (`libraries.tasks.generate_release_report`) with statistics: commits,
   contributors, a word cloud.

So Boost's pipeline is: maintainers write per-library entries (long form in
their docs, short form in the release notes), release managers write General
Notes and the announcement, and the site derives the "What's new" summary by
LLM with human approval.

## 5. Proposal for webcpp

webcpp facts this builds on (`~/dev/webcpp`, `~/.buzz/PLANS/WEBCPP_DESIGN.md`):
a superproject `webcpporg/webcpp` with libraries as submodules under
`libs/<name>` (today pratt, wasi, xactor, xstate; config planned as its own
repository), branch `main` only, docs in AsciiDoc built by b2 with
Asciidoctor.js, one page per library under `libs/<name>/doc/`, the
superproject's page `doc/index.adoc`, GitHub Actions CI, b2 as the only
build.

### 5.1 Where the text lives

| What | Where | Boost's equivalent |
| --- | --- | --- |
| A library's changelog | `libs/<name>/doc/changelog.adoc`, one section per webcpp release, newest first, included as the last chapter of the library's page | Unordered's `changes.adoc`, Mp11's `changelog.adoc` |
| The release notes of the collection | `doc/releases/<version>.adoc` in the superproject (`doc/releases/0.1.0.adoc`), published on the site as "webcpp 0.1.0", with `doc/releases/template.adoc` | `website-v2-docs/release-notes/boost_1_92_0.adoc` and `template.adoc` |
| The "What's new" post | `doc/releases/<version>.whats-new.md`, used verbatim as the GitHub Release body | the "What's new in 1.92.0" card and the announcement e-mail |

**One source per fact.** Boost keeps two hand-written copies (library
changelog and release-notes entry) and they drift (JSON's duplicated block,
URL and Decimal with no library changelog at all). webcpp keeps one: each
library's release section is the only place its changes are written, and
`doc/releases/<version>.adoc` includes it with AsciiDoc tagged regions, so
nothing is copied and nothing is generated into the tree:

```
// libs/xstate/doc/changelog.adoc
= Changelog

== webcpp 0.2.0
// tag::0.2.0[]
* *Breaking change:* `machine::send` returns `result<void>`; check it where you ignored it.
* Added `after` transitions with a caller-provided clock (webcpp_gh:pr[xstate,41]).
* Fixed a missed exit action when a parallel state is left (webcpp_gh:issue[xstate,37]).
* Deprecated `machine::state_value`; use `snapshot().value`.
* Updated the documentation of guards and actions.
// end::0.2.0[]
```

```
// doc/releases/0.2.0.adoc
== Updated Libraries

=== link:../../libs/xstate/[xstate]

include::../../libs/xstate/doc/changelog.adoc[tag=0.2.0]
```

The tag opens after the section heading, so the include pulls only the
bullets. Each library is a `===` heading in the release notes rather than
Boost's top-level bullet, because an included `*` list cannot be nested
under a bullet of the including file.

The superproject file holds only what belongs to the collection, hand
written: Version and documentation link, General Notes (optional), New
Libraries, Updated Libraries (includes), Removed Libraries (the owner wants
what went out, Boost has no such section), Updated Tools (optional:
`tools/`), Targets Tested (native toolchains, wasip2, wasip3, emscripten,
in place of Compilers Tested), Known Issues (optional, with patches),
Acknowledgements.

A new library has no release section in its changelog; it gets the Boost
one-liner in the release notes: "* xstate: XState v5's state machines and
actors for C++20, from <author>." and its changelog starts at the release
after.

### 5.2 The rule for AGENTS.md (agents may write changelogs)

Proposed text, Boost's best practices made checkable:

- Every change to a library that a user can see adds a bullet to the
  library's `doc/changelog.adoc` under the next release's tagged section, in
  the same commit as the change. Create the section if it is not there.
- One bullet per change, one sentence, about ten words, at most about 25;
  what the user sees, not how it was done. Start with a past-tense verb:
  Added, Changed, Fixed, Removed, Deprecated, Improved, Updated.
- Breaking changes come first and start with `*Breaking change:*`, followed
  by what the user must change.
- Deprecations say what replaces the deprecated thing and that it will be
  removed in a future release.
- A fixed bug names the symptom the user saw, not the cause, with its link
  `(webcpp_gh:issue[<lib>,<n>])` when an issue exists; a pull request is
  `(webcpp_gh:pr[<lib>,<n>])`.
- All documentation work of a release is one bullet.
- At most ten bullets per library per release: the first three are the most
  important; merge or drop the rest. Longer explanations go into the
  library's documentation, which the bullet links to.
- Internal changes (CI, lint, refactoring with no visible effect) get no
  bullet.
- Never use the em dash. Never rewrite a released section; a correction is
  a new bullet in the next release.

`webcpp_gh:issue[...]` and `webcpp_gh:pr[...]` are the Boost macros
renamed: a small Asciidoctor.js inline-macro extension under `tools/doc/`
rendering `#37` and `PR#41` linked to `https://github.com/webcpporg/<lib>/issues/<n>`
and `.../pull/<n>`. Until it exists, the plain form
`https://github.com/webcpporg/xstate/issues/37[#37]` is the fallback.

### 5.3 The "What's new" post

Assembled at release time, by an agent, from `doc/releases/<version>.adoc`,
and approved by the owner before the tag, which is Boost's own pipeline
(LLM summary plus approval) with the announcement folded in:

```
# webcpp 0.2.0

webcpp 0.2.0 is out: <one sentence on what webcpp is>.

<New libraries, one line each, copied from New Libraries.>

## Highlights
- **Performance**: <one sentence, at most 20 words>.
- **Dependencies**: ...
- **Reliability**: ...
- **Developer experience**: ...

## Libraries
- **xstate**: <the first one to three bullets of its section>.
- ...

Full release notes: <site link>. Checksums: `webcpp-0.2.0.tar.gz.txt`.
```

The Highlights follow the prompt of `versions/tasks.py`: at most 5 bullets,
a bold category and one sentence of at most 20 words each, in the order New
libraries, Performance, Dependencies, Security & reliability, Developer
experience, omitting empty categories and inventing nothing. Unlike Boost's
card, it may name libraries, since webcpp has a handful. `b2 whats-new`
writes the skeleton (title, new-library lines, the first bullets of every
library, links); the agent writes the Highlights; the file is committed and
`gh release create --notes-file` publishes it.

### 5.4 Version, archive, tags

**Version header.** `<webcpp/version.hpp>` in the config library
(`libs/config/include/webcpp/version.hpp`, repository `webcpporg/config`), as
`boost/version.hpp` lives in Boost.Config, with Boost's formula:

```
//  WEBCPP_VERSION % 100 is the patch level
//  WEBCPP_VERSION / 100 % 1000 is the minor version
//  WEBCPP_VERSION / 100000 is the major version
#define WEBCPP_VERSION 100          // 0.1.0 = 0 * 100000 + 1 * 100 + 0
#define WEBCPP_LIB_VERSION "0_1"     // "x_y", and "_z" only when z != 0
```

0.1.0 is 100, 0.1.1 is 101 with "0_1_1", 0.2.0 is 200, 1.0.0 is 100000.
Unlike Boost, which repeats the version by hand in `Jamroot` and
`CMakeLists.txt`, the header is the **only** place: the superproject
Jamroot already reads Boost's version from `boost/version.hpp`, and reads
webcpp's the same way into `constant WEBCPP_VERSION`. As Boost does, the
header is bumped to the next version in the commit right after the tag, so
`main` always names the release in preparation, and that commit also adds
`doc/releases/<next>.adoc` from the template (Boost's 1.93 stub appeared
the day 1.92 shipped).

**Archive.** Follow the GitHub form, since webcpp publishes on GitHub and
the tag names it: tag `webcpp-0.1.0`, archive `webcpp-0.1.0.tar.gz`, top
directory `webcpp-0.1.0/`, checksum file `webcpp-0.1.0.tar.gz.txt` written
by `sha256sum` (checkable with `sha256sum -c`), attached to the GitHub
Release `webcpp-0.1.0`. Boost's underscore form `boost_1_92_0` is a legacy of
archives.boost.io and adds nothing for a GitHub-only release. Contents: the
superproject at the tag with every submodule, **without** `.git`, `.github`,
`.gitmodules`, `.gitignore` and other dot files (`MakeBoostDistro.py`'s
rule, cleaner than the GitHub workflow's), **with** the built documentation
(`doc/html` and each `libs/<name>/doc/html`), as `boost_1_92_0.tar.gz`
ships `doc/html`. Headers stay in `libs/<name>/include/`; webcpp's build
finds them there, so the flattening of Boost's b2 archive is not needed.
Beyond Boost, and cheap: make it reproducible (`tar --sort=name
--mtime=<tag commit date> --owner=0 --group=0 --numeric-owner`, `gzip -n`),
so anyone can rebuild it from the tag and compare the hash. Boost signs
nothing; whether webcpp signs (as an earlier private project does with its builder SSH keys,
D-058 there) is a separate decision for the owner.

**Tags.** As `publish_release.py` does: the same tag `webcpp-0.1.0` on the
superproject and on every library repository at the commit the superproject
records, so a library's release state is found by tag. No separate library
versions: webcpp has one version (a port records its original's version
elsewhere). Betas, when wanted, `webcpp-0.2.0.beta1`; release candidates are
not tagged; a re-packaging with no code change, `webcpp-0.2.0-1`. Boost's
tags are lightweight and unsigned; webcpp should use annotated tags, signed
if the owner decides to sign.

### 5.5 What b2 and tools generate, and the checks that keep them honest

| Target | Does |
| --- | --- |
| `b2 dist` | Builds the docs, then `webcpp-<version>.tar.gz` and its `.txt` from a clean export at `HEAD`, reproducibly, into `dist/`; the version comes from `webcpp/version.hpp` |
| `b2 whats-new` | Writes the skeleton of `doc/releases/<version>.whats-new.md` from the release notes |
| `tools/release/tag.py` (or `make tag`, needs the terminal) | Checks everything below, then tags the superproject and every submodule `webcpp-<version>` and pushes; refuses if `main` is not clean |
| CI workflow on tag push `webcpp-*` | As boostorg/boost's `release-cmake.yml`: checks out with submodules, runs `b2 dist`, checks the hash is the one committed or reproduced, creates the GitHub Release with the archive, the `.txt` and the What's new as body |
| post-release step of the same tool | Bumps `version.hpp` and opens `doc/releases/<next>.adoc` from the template |

Checks, as one `b2 release-check` in `b2 ci` (soft on ordinary commits,
hard in `tag.py`):

1. **Every changed library is in the notes.** For each `libs/<name>`, compare
   the commit the superproject recorded at the last tag `webcpp-<prev>` with
   the current one (`git diff --quiet <old> <new> -- include src` and the
   other user-visible paths). A library whose public tree changed must have a
   tagged section `<version>` in its changelog with at least one bullet, and
   `doc/releases/<version>.adoc` must include it. A library absent from the
   previous tag's `.gitmodules` must be under New Libraries; one present then
   and gone now must be under Removed Libraries. This is the check the owner
   asked about, and Boost has no equivalent.
2. **The version agrees.** `WEBCPP_VERSION` and `WEBCPP_LIB_VERSION` encode
   the same version; `doc/releases/<that version>.adoc` exists; at tag time
   the tag name is `webcpp-<that version>` and no such tag exists yet.
3. **The format holds.** Libraries sorted; every bullet of a release section
   one sentence starting with an allowed verb or `*Breaking change:*`; at
   most ten bullets per library; no em dash; links through `webcpp_gh:`;
   released sections unchanged since their tag (`git diff webcpp-<prev> --`
   restricted to those regions).
4. **The archive is what it claims.** `b2 dist` output has the top directory
   `webcpp-<version>/`, no `.git` or dot file, the `version.hpp` of that
   version, built docs for every library, and its SHA-256 equals the `.txt`.

## Sources

- https://github.com/boostorg/website-v2-docs/tree/develop/release-notes (1.90, 1.91, 1.92, 1.93 stub, template), commit `8f958ca` of 2026-10-07
- https://github.com/boostorg/website-v2-docs/blob/develop/contributor-guide/modules/ROOT/pages/release-notes.adoc
- https://github.com/boostorg/website-v2-docs/blob/develop/contributor-guide/modules/ROOT/pages/release-process.adoc
- https://github.com/boostorg/website/tree/master/feed/history (1.82, 1.83, 1.85, 1.87, 1.88, 1.89 qbk)
- https://github.com/boostorg/website/tree/master/patches
- https://github.com/cppalliance/asciidoctor-boost (macros)
- https://github.com/boostorg/website-v2/blob/develop/versions/tasks.py, `versions/models.py`, `versions/releases.py`, `versions/views.py`
- https://www.boost.org/releases/1.92.0/
- https://lists.boost.org/archives/list/boost-announce@lists.boost.org/ (1.90.0, 1.91.0, 1.92.0 releases; 1.91.0 and 1.92.0 betas)
- https://github.com/boostorg/release-tools: `ci_boost_release.py`, `MakeBoostDistro.py`, `publish_release.py`, `README` (commit `ee72a1a`)
- https://github.com/boostorg/boost/releases and `.github/workflows/release-cmake.yml`
- https://archives.boost.io/release/1.92.0/ and https://archives.boost.io/beta/1.92.0.beta1/
- https://github.com/boostorg/config/blob/develop/include/boost/version.hpp and its commit history
- Library changelogs: boostorg/json `CHANGELOG.adoc`, boostorg/unordered `doc/modules/ROOT/pages/changes.adoc`, boostorg/mp11 `doc/mp11/changelog.adoc`, boostorg/hash2 `doc/hash2/changes.adoc`, boostorg/beast `doc/qbk/release_notes.qbk` and `CHANGELOG.md`, boostorg/asio `doc/history.qbk`, boostorg/decimal GitHub releases
