---
title: "The webcpp site, increments 1 to 3: Implementation Plan"
tags: [webcpp, site, docs, emscripten, plan]
status: active
created: 2026-10-10
---

# The webcpp site, increments 1 to 3: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `b2 site` builds webcpp's whole site into `bin/site/`, calling the site's npm
scripts for its JavaScript parts. The preview is published
at the organisation's root, https://webcpporg.github.io/, from the site's own repository,
and the old address redirects there. The site's state machines run on webcpp::xstate
compiled to WebAssembly, and its first example, the pratt calculator, plays in the bridge
visualizer.

**Architecture:**
- **The site's repository:** a new git repository, `webcpporg/webcpporg.github.io`, checked
  out as the superproject's submodule `site/`. It is a webcpp project like any user's:
  - React components with zod props, its design system;
  - pages in OpenUI Lang, rendered to static HTML at build time;
  - the C++ of its own wasm and of its examples, built for emscripten.
- **The superproject owns the assembly.** `b2 site`:
  - renders the site's pages;
  - takes every library's page and the test report;
  - writes `bin/site/`, checking every link, as `tools/ci/assemble.py` does today;
  - the CI pushes the result to the site repository's `gh-pages` branch.
- **The site is a JavaScript application and a PWA.**
  - Each page is a small shell carrying its OpenUI Lang source.
  - The browser renders it with OpenUI's React renderer, hydrating the same tree the build
    rendered for the first paint.
  - Mobile first.
  - A service worker caches every file, every wasm included, once the reader agrees.
- **Increments 2 and 3** add:
  - xstate's emscripten target;
  - the site's wasm, which runs `machines/player.json` and `machines/install.json` (the
    PWA's install flow);
  - the trace, the visualizer and the calculator.

**Tech Stack:**
- C++20, b2 5.5.3, Boost 1.92, Emscripten 6.0.11, node 26.7.0, Python 3.14.
- The site's node packages, pinned by `package-lock.json`:
  - `react` 19.3.0, `react-dom` 19.3.0, `@openuidev/react-lang` 0.4.1, `zod` 4.6.5;
  - `esbuild` 0.28.2, `typescript` 7.0.2, `@types/react` 19.3.0, `@types/react-dom` 19.3.0,
    `html-validate` 11.16.2 (MIT, the check that every page is valid HTML);
  - `@resvg/resvg-js` 2.6.2 (MPL-2.0, a build tool that never ships): the PNG icons from the
    logo's SVG;
  - `xstate` 5.33.2, a dev dependency only: the oracle of the site's machines in tests.
- Chrome for Testing `chrome-headless-shell` 155.0.8059.39, driven over DevTools as
  `libs/trystero/test/oracle/interop/run.mjs` drives it.

**Spec:** `~/.buzz/PLANS/WEBCPP_SITE_SPEC.md` (approved 2026-10-10).
- The rulings are in `~/.buzz/PLANS/WEBCPP_DESIGN.md` (2026-10-10 entries).
- The research is in `~/.buzz/RESEARCH/WEBCPP_SITE_REFERENCES.md`, with the
  OpenUI static-render spike.

**Not in this plan** (spec section 12, increments 4 to 8):
- trystero's JavaScript API and Trystero's examples;
- the state machine, "C++ calls the browser" and "on the server" examples;
- the measured sizes of section 8, which come with the first JavaScript-facing library;
- the page that plays the player's own machine in the visualizer, which comes with the state
  machine example;
- Google search with an engine id;
- the release site at webcpp.org.

The home page and the doors say what is coming, linking the roadmap; they never show a
number or an example that does not exist.

## Global Constraints

**Repositories and paths:**
- `~/dev/webcpp` is the superproject (`git@github.com:webcpporg/webcpp.git`, branch `main`).
- `~/dev/webcpp/site` is the site's own git repository, created in Task 1, branch `main`.
  - Its remote, `git@github.com:webcpporg/webcpporg.github.io.git`, is created by the
    controller at gate G1.
  - It becomes the superproject's submodule `site` in Task 10.
  - Until then the superproject ignores `site/` through `.git/info/exclude`, never through
    a tracked `.gitignore` line.
- `~/dev/webcpp/libs/*` are the libraries' own repositories (submodules). A change there is
  a commit in that repository, then a commit in the superproject that moves the pointer.
- Machine-local tools: `~/dev/webcpp/.local/` (git-ignored): `user-config.jam`, `emsdk/`
  (6.0.11), `mrdocs/`, `wasi-sdk/`.
- Every b2 command runs from `~/dev/webcpp`, as
  `env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam ...`
  (the plan writes `$B2` for that).
- A verdict is `-a`.
- Each lane has its own `--build-dir`.
- No cap on concurrent builds (owner, 2026-10-11): a test that fails under load is flaky and is fixed, never protected by running less at once.

**Commits and text:**
- Every commit-writing git command is
  `git -c user.name=R -c user.email=github.overboard312@passmail.net commit ...`, with no trailer of
  any kind and never an agent co-author.
- After each commit, `git log -1 --format='%an %ae %cn %ce'` prints
  `R github.overboard312@passmail.net R github.overboard312@passmail.net`, and
  `git log -1 --format=%B | git interpret-trailers --parse` prints nothing.
- Stage by path. Never `git add -A`, `git add .` or `git add libs` in the superproject.
- Everything written to a file is English. No em dash (U+2014). The word the lint forbids
  (`tools/lint/rules.py`) appears nowhere.
- Every source file of the site starts with the licence notice, as the superproject's files
  do (`// Copyright (c) 2026 WebCpp.org` and the BSL-1.0 lines; `#` for Python and Jam,
  `<!-- -->` for HTML).

**Outward actions (the controller alone, each at its gate, never a subagent):**
- **G1:** create `webcpporg/webcpporg.github.io` (public, BSL-1.0, no template), set its
  Pages to deploy from the branch `gh-pages`, create the deploy key pair, add the public
  half to the site repository with write access, and add the private half to the
  superproject as the Actions secret `SITE_DEPLOY_KEY`. Owner's agreement required first.
- **G2:** push the site repository, each library repository changed, and the superproject.
  Owner's agreement required first.
- **G3:** the owner chooses the logo among Task 8's concepts.
- A subagent never pushes, never creates a repository, never changes a GitHub setting, and
  never prints an environment variable (no `env`, `export`, `printenv`, `set`).

**The site's rules:**
- **Static files, no server.** No runtime API, no cookie, no analytics. The one
  third-party script is Google's search element, which this plan does not add.
- **The site is a JavaScript application (the owner, 2026-10-10).**
  - All content is OpenUI Lang, rendered by OpenUI. The hand-written HTML is only the
    page's shell, which `renderPage` writes.
  - The build renders each page's tree with the same renderer for the first paint, and
    the client hydrates it.
  - A browser without JavaScript gets the one `<noscript>` message the shell carries.
- **Mobile first.** Styles are written from 360 px up. Every visual pass looks at 360,
  768, 1280 and 1440 px, light and dark.
- **The build goes through b2, the JavaScript through npm.**
  - Every JavaScript step is an npm script of the site's `package.json` (`build`, `test`,
    `typecheck`, `dev`, `serve`).
  - b2 runs them through the one rule `site.npm` and never repeats their logic.
  - Only `b2 site`'s output is ever deployed.
- **No network at build or test time** beyond `npm ci` from the lockfile.
- **The client bundle is built with `process.env.NODE_ENV` defined as `"production"`.**
  `@openuidev/react-lang`'s web entry mounts a devtools widget fetched from a CDN when it is
  `"development"`. Task 2's test proves that no `@openuidev/devtools` and no `https://` URL
  of a CDN reach the bundle.
- **Every link inside the site is relative.** A path from the server's root (`/x`) is
  refused by the link check, as today, so the same output serves at a root and under a
  prefix.
- **No number on a page is typed by hand.** Counts and versions come from the build's
  inputs.
- **SEO is part of done (the owner, 2026-10-10).** Every page has the head of the spec's
  SEO section, checked by tests:
  - title, description, canonical, robots, Open Graph, Twitter card and JSON-LD;
  - one `<h1>`;
  - a social card.

  The queries of `seo/queries.json` must be answered by their pages.
- **Browser storage** (`localStorage`) is read and written only inside `try`/`catch`, and
  the site works when it throws.

**Names fixed by this plan:**

| Name                                             | What it is                                                                  |
| ------------------------------------------------ | --------------------------------------------------------------------------- |
| `b2 site`                                        | the superproject's target that writes `bin/site/`                           |
| `-sWEBCPP_SITE=preview\|release`                 | the channel, `preview` by default                                           |
| `-sWEBCPP_SITE_VERSION=<v>`                      | the release's version, required by `release`                                |
| `-sWEBCPP_SITE_DOCS=<dir>`                       | pages already built by `b2 doc -sWEBCPP_INDEX=site` (CI)                    |
| `-sWEBCPP_SITE_REPORT=<dir>`                     | the test report already written (CI)                                        |
| `/webcpp/site//pages`                            | the site's rendered pages, in the site's build directory                    |
| `/webcpp/site//test`                             | the site's own tests                                                        |
| `tools/site/catalogue.py`                        | writes `libraries.json` from every library's meta and `b2 declared-targets` |
| `tools/site/redirect.py`                         | writes the redirect site of the old address                                 |
| `tools/ci/assemble.py`                           | lays out `bin/site/` and checks every link (extended)                       |
| `site.npm <script> : <arguments>`                | the b2 rule (`tools/site/site.jam`) that runs one of the site's npm scripts |
| `b2 site-serve`                                  | builds `bin/site/` and serves it on 127.0.0.1                               |
| `precache.json`, `sw.js`, `manifest.webmanifest` | the PWA's files at the site's root                                          |

## Review Focus

1. **A library whose `meta/libraries.json` lacks an optional field or has `"port-of":
   null`** (a library of webcpp's own) appears on the list with "webcpp's own" in place of
   an original. A missing required field (`key`, `name`, `description`) fails
   `catalogue.py`, naming the file and the field. Task 3 tests all three.
2. **A reader who declines the installation, or a browser that refuses the wasm or the
   service worker,** still uses the site from the network:
   - declining is remembered for that version, and the question is not asked again on the
     next page;
   - a refused wasm makes the visualizer say in text that the example could not start, and
     why;
   - a version change of `precache.json` re-downloads only the files whose hash changed.

   Task 14 tests the first and the third; Task 16 tests the second.
3. **A stale `bin/site/`** from an earlier run, holding a page since removed, never survives
   into the new site: `b2 site` writes into an empty directory. Task 5 tests a page removed
   between two runs.
4. **Storage and sound refused**: `localStorage` throwing (private mode) and an
   `AudioContext` that cannot start (autoplay policy, no audio device) never throw out of
   the player; the mute control still works for the session. Task 15 tests both.
5. **The same site served under a prefix** (`/webcpp/` during the move, a fork's
   `/<repo>/`) works: no link names the server's root. Task 5's link check refuses `/x`,
   and Task 4 writes `sitemap.xml` with the channel's absolute base, the one place
   absolute URLs belong.

---

## Task 0: The ledger and the baseline

**Files:**
- Create: `~/dev/webcpp/.superpowers/sdd/WEBCPP_SITE_PLAN/progress.md`

- [ ] **Step 1:** Create the ledger with the first line
  `# SDD ledger - plan: ~/.buzz/PLANS/WEBCPP_SITE_PLAN.md`.
- [ ] **Step 2: Measure the baseline.** Record in the ledger:
  - `git -C ~/dev/webcpp rev-parse HEAD`;
  - each library's submodule commit (`git -C ~/dev/webcpp submodule status`);
  - the output of `$B2 -d0 declared-targets`;
  - the number of pages today's site has. Build it with
    `$B2 -a --build-dir=bin/b-doc doc -sWEBCPP_INDEX=site`, then
    `python3 tools/ci/assemble.py --docs . --report tools/report/samples --out $TMPDIR/site-before`.
    If `tools/report/samples` is not a report directory, use the smallest report
    `tools/report/report.py` writes from `tools/report/samples`, and record the command.

  Expected: `assemble.py` prints `wrote ..., N pages, 5 libraries, every link resolving`.
  N is the baseline.
- [ ] **Step 3: Record the published URLs to move.**
  `grep -rln "webcpporg.github.io/webcpp" README.md AGENTS.md doc libs/*/README.md libs/*/doc`.
  Expected: the README and AGENTS.md of the superproject, `doc/index.adoc`, and the README
  and `doc/testing.adoc` of xactor, xstate, pratt, wasi and trystero. Task 11 changes them.

## Task 1: The site's repository and its node packages

**Files:**
- Create (in `~/dev/webcpp/site`, a new git repository):
  - `README.md`, `AGENTS.md`, `LICENSE_1_0.txt` (a copy of the superproject's);
  - `.gitignore` (`node_modules/`, `.node-modules/`, `build/`);
  - `package.json`, `package-lock.json`;
  - `tsconfig.json`;
  - `Jamfile`;
  - `test/package_test.mjs`.
- Modify: `~/dev/webcpp/.git/info/exclude` (the line `/site/`).

**Interfaces:**
- Produces:
  - the project `/webcpp/site`;
  - its target `node-modules` (the packages installed by `tools/node/install.py`);
  - the npm scripts:
    - `build`, Task 2;
    - `test`, which runs `node --test test/`;
    - `typecheck`;
    - `dev` and `serve`, Task 5.

- [ ] **Step 1: Create the repository.**

```bash
mkdir -p ~/dev/webcpp/site && cd ~/dev/webcpp/site && git init -b main
cp ../LICENSE_1_0.txt .
echo /site/ >> ~/dev/webcpp/.git/info/exclude
```

- [ ] **Step 2: Write `package.json`,** exact versions, no ranges:

```json
{
    "name": "webcpp-site",
    "private": true,
    "type": "module",
    "license": "BSL-1.0",
    "scripts": {
        "build": "node tools/build.mjs",
        "test": "node --test test/",
        "typecheck": "tsc --noEmit",
        "dev": "node tools/dev.mjs",
        "serve": "node tools/serve.mjs"
    },
    "dependencies": {
        "@openuidev/react-lang": "0.4.1",
        "react": "19.3.0",
        "react-dom": "19.3.0",
        "zod": "4.6.5"
    },
    "devDependencies": {
        "@resvg/resvg-js": "2.6.2",
        "@types/react": "19.3.0",
        "@types/react-dom": "19.3.0",
        "esbuild": "0.28.2",
        "html-validate": "11.16.2",
        "typescript": "7.0.2",
        "xstate": "5.33.2"
    }
}
```

  Then run `npm install --package-lock-only` and `npm ci`.

- [ ] **Step 3: Write the failing test** `test/package_test.mjs`, which pins the versions:

```js
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

import assert from 'node:assert/strict'
import {readFileSync} from 'node:fs'
import test from 'node:test'

const lock = JSON.parse(readFileSync(new URL('../package-lock.json', import.meta.url)))
const pinned = {
    'node_modules/@openuidev/react-lang': '0.4.1',
    'node_modules/react': '19.3.0',
    'node_modules/react-dom': '19.3.0',
    'node_modules/zod': '4.6.5',
    'node_modules/esbuild': '0.28.2',
    'node_modules/html-validate': '11.16.2',
    'node_modules/@resvg/resvg-js': '2.6.2',
    'node_modules/xstate': '5.33.2',
}

test('the lockfile pins the versions the plan names', () => {
    for (const [path, version] of Object.entries(pinned)) {
        assert.equal(lock.packages[path]?.version, version, path)
    }
})
```

- [ ] **Step 4:** Run it before the lockfile exists (move it aside):
  `npm test`. Expected: FAIL, `ENOENT ... package-lock.json`. Put the lockfile back, run
  `npm test`. Expected: PASS, 1 test.
- [ ] **Step 5: Write the `Jamfile`,** declaring the project and its node packages the way
  `tools/doc/Jamfile` declares the documentation's:

```jam
# Copyright (c) 2026 WebCpp.org
#
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# https://www.boost.org/LICENSE_1_0.txt)
#
# The site: its pages, rendered from OpenUI Lang into static HTML, and its
# own tests. b2 site, at the superproject, assembles them with every
# library's page into bin/site/.

project /webcpp/site ;

make node-modules.stamp : package.json package-lock.json : @webcpp-doc%install-packages-in
  : <webcpp-boost-check>off ;
alias node-modules : node-modules.stamp : <webcpp-boost-check>off ;
explicit node-modules.stamp node-modules ;
always node-modules.stamp ;
```

  `webcpp-doc%install-packages` takes the directory from `$(.here)`, the doc tools' own.
  Add to `tools/doc/doc.jam` a rule `install-packages-in`, which sets `HERE` from the
  directory of the Jamfile that uses it, and use it in both Jamfiles, so that the
  superproject keeps one installer. The site's `make` above already names it.

- [ ] **Step 6: Register the project in the superproject's `Jamroot`,** after the libraries'
  loop:

```jam
# The site, site/ (webcpporg/webcpporg.github.io), when it is checked out:
# a project like a user's, which b2 site assembles (tools/site/site.jam).
if [ glob site/Jamfile ]
{
    use-project /webcpp/site : site ;
}
```

  Run `$B2 /webcpp/site//node-modules`. Expected: `installed`, and
  `site/node_modules` a link into `site/.node-modules/<digest>/`.

- [ ] **Step 7: Write the site's `README.md` and `AGENTS.md`.**
  - **README:**
    - what the repository is: webcpp's site, built with webcpp;
    - then two sections, one per reader (the spec's "Two ways in"), which Task 5 completes
      when `dev`, `serve` and `b2 site-serve` exist:
      - **"For JavaScript developers"**: path 1, npm alone for the examples and pages;
        path 2, the whole project, b2 included, taught step by step with exact versions
        and commands;
      - **"For C++ developers"**: b2 alone (`b2 site`, `b2 site-serve`, the test
        targets), with node among the prerequisites as a tool b2 runs. No npm command, no
        `package.json`, no bundler named.
  - **AGENTS.md:** only what is specific to the site:
    - pages are OpenUI Lang;
    - components are React with zod props;
    - the client bundle is production;
    - links are relative;
    - nothing on a page is typed that the build knows;
    - point to the superproject's AGENTS.md for everything else.
- [ ] **Step 8: Commit** in `~/dev/webcpp/site`, and in the superproject (`Jamroot`,
  `tools/doc/doc.jam`):

```bash
git -c user.name=R -c user.email=github.overboard312@passmail.net commit -m "The site's repository: its node packages, pinned, and its project"
```

  Superproject:
  `"Jamroot: the site's project when site/ is checked out; doc.jam: install-packages-in, the node installer for any directory"`.

## Task 2: The component library and the renderer

**Files:**
- Create (site):
  - `src/components/*.tsx`: one file per component;
  - `src/library.ts`: `createLibrary` over them;
  - `src/context.ts`: the `SiteContext` type and React context;
  - `src/server.tsx`: `renderPage`;
  - `src/client.tsx`: the browser's entry. It reads the page's OpenUI Lang from the shell
    and hydrates the prerendered tree with OpenUI's renderer;
  - `tools/build.mjs`: the `build` script, with the esbuild bundles (server and client) and
    the render loop;
  - `assets/site.css`;
  - `test/render_test.mjs`.

**Interfaces:**
- Produces, from `build/server.mjs` (bundled from `src/server.tsx`):

```ts
export type Library = {
    key: string
    name: string
    description: string
    category: string[]
    targets: string[]                      // e.g. ["emscripten", "native", "wasip2", "wasip3"]
    port_of: {name: string; language: string; version: string; url: string; licence: string} | null
    page: string                           // "libs/<key>/"
    repository: string                     // "https://github.com/webcpporg/<key>"
}
export type SiteContext = {
    channel: 'preview' | 'release'
    commit: string                         // 40 hex
    date: string                           // YYYY-MM-DD, the commit's date
    version: string | null                 // the release's, null on preview
    catalogue: Library[]
    root: string                           // from this page to the site's root: "" or "../" ...
    path: string                           // this page's path: "" for home, "libraries/" ...
}
export class PageError extends Error { file: string; line: number }
// A whole HTML document: the shell (head, manifest link, <noscript>), the page's OpenUI Lang
// in <script type="text/openui" id="page-source">, the context in
// <script type="application/json" id="site-context">, the tree OpenUI renders, prerendered
// inside <div id="root">, and <script type="module" src="{root}assets/site.js">.
export function renderPage(file: string, source: string, context: SiteContext): string
export function renderFragment(name: 'docs-header', context: SiteContext): string        // HTML for assemble.py
```

- The components and their positional props, in this order, which is the OpenUI Lang call
  shape:

| Component | Props | Renders |
| --- | --- | --- |
| `Page(title, description, body)` | string, string, array of blocks | the document's `<main>`; the shell adds head, header, band and footer |
| `Hero(title, subtitle, actions)` | string, string, array of `Link` | the home's first block |
| `Section(title, body)` | string, array of blocks | `<section>` with `<h2>` |
| `Text(text)` | string | `<p>`; `**x**` bold and `` `x` `` code, nothing else |
| `Code(language, source)` | string, string | `<pre><code class="language-...">` |
| `Link(text, href)` | string, string | `<a>`; an `href` without a scheme is made relative to the root |
| `Doors()` | none | the three doors, C++, JavaScript, AI |
| `Targets()` | none | the three places the same code runs |
| `LibraryTable()` | none | the table of section 5, from `context.catalogue` |
| `Roadmap(items)` | array of string | a list of what is coming, each a line |

- [ ] **Step 1: Write the failing test** `test/render_test.mjs`:

```js
// (licence notice)
import assert from 'node:assert/strict'
import test from 'node:test'
import {renderPage, PageError} from '../build/server.mjs'

const context = {
    channel: 'preview', commit: '0123456789abcdef0123456789abcdef01234567', date: '2026-10-10',
    version: null, root: '', path: '',
    catalogue: [{
        key: 'pratt', name: 'pratt', description: 'A Pratt parser engine.', category: ['Parsing'],
        targets: ['emscripten', 'native', 'wasip2', 'wasip3'], port_of: null,
        page: 'libs/pratt/', repository: 'https://github.com/webcpporg/pratt',
    }],
}
const page = `root = Page("Home", "Header-only C++20 libraries for the web: compiled to WebAssembly for the browser with Emscripten, to WASI for servers, or natively.", [hero, table])
hero = Hero("Where C++ and the web meet", "C++ libraries for the web.", [docs])
docs = Link("The libraries", "libraries/")
table = LibraryTable()
`

test('a page is a shell that carries its OpenUI Lang and the tree OpenUI rendered', () => {
    const html = renderPage('pages/index.oui', page, context)
    assert.match(html, /^<!doctype html>/)
    assert.ok(html.includes('<script type="text/openui" id="page-source">'))
    assert.ok(html.includes('hero = Hero("Where C++ and the web meet"'))
    assert.match(html, /<div id="root"><[^>]+>[\s\S]*<h1[^>]*>Where C\+\+ and the web meet<\/h1>/)
    assert.match(html, /<a href="libraries\/">The libraries<\/a>/)
    assert.match(html, />pratt<\/a>/)
    assert.match(html, /webcpp's own/)
    assert.match(html, /<script type="module" src="assets\/site\.js"><\/script>/)
    assert.match(html, /<noscript>[^<]*needs JavaScript[^<]*<\/noscript>/)
})

test('the page source in the shell cannot close its script element', () => {
    const tricky = 'root = Page("x", "</script><b> Header-only C++20 libraries for the web: compiled to WebAssembly for the browser with Emscripten, to WASI for servers, o", [])\n'
    const html = renderPage('pages/x.oui', tricky, context)
    assert.equal(html.split('</script>').length - 1, html.match(/<script/g).length)
})

test('the head carries every SEO tag the spec names', () => {
    const html = renderPage('pages/index.oui', page, context)
    for (const tag of [
        /<title>[^<]{1,60}<\/title>/, /<meta name="description" content="[^"]{120,160}">/,
        /<link rel="canonical" href="https:\/\/webcpporg\.github\.io\/">/,
        /<meta name="robots" content="index, follow">/, /<html lang="en"/,
        /<meta property="og:title"/, /<meta property="og:description"/, /<meta property="og:url"/,
        /<meta property="og:type"/, /<meta property="og:site_name" content="webcpp">/,
        /<meta property="og:image" content="https:\/\/[^"]+\.png">/, /<meta property="og:image:alt"/,
        /<meta name="twitter:card" content="summary_large_image">/,
        /<script type="application\/ld\+json">/]) {
        assert.match(html, tag)
    }
    const graph = JSON.parse(html.match(/<script type="application\/ld\+json">([^<]*)<\/script>/)[1])['@graph']
    assert.deepEqual(graph.map(n => n['@type']).sort(), ['BreadcrumbList', 'Organization', 'WebPage', 'WebSite'])
    assert.equal((html.match(/<h1[ >]/g) || []).length, 1)
})

test('a description outside 120 to 160 characters fails with its file', () => {
    assert.throws(() => renderPage('pages/short.oui', 'root = Page("x", "Too short.", [])\n', context),
        e => e instanceof PageError && e.file === 'pages/short.oui' && /description/.test(e.message))
})

test('the preview band names the commit and its date', () => {
    const html = renderPage('pages/index.oui', page, context)
    assert.match(html, /Preview of main at <code>0123456<\/code>, 2026-10-10/)
})

test('a page that names an unknown component fails with its file and line', () => {
    assert.throws(() => renderPage('pages/bad.oui', 'root = Page("x", "y", [a])\na = Nope("z")\n', context),
        e => e instanceof PageError && e.file === 'pages/bad.oui' && e.line === 2 && /Nope/.test(e.message))
})

test('a link from the server root is refused', () => {
    assert.throws(() => renderPage('pages/x.oui', 'root = Page("x", "y", [l])\nl = Link("a", "/libraries/")\n', context),
        e => e instanceof PageError && /relative/.test(e.message))
})
```

- [ ] **Step 2:** Run `npm run build -- --bundle && npm test`.
  Expected: FAIL. The build script does not exist yet.
- [ ] **Step 3: Write the components.** Each file has the licence notice and one
  `defineComponent`. For example, `src/components/text.tsx`:

```tsx
import {defineComponent} from '@openuidev/react-lang'
import {z} from 'zod'

/** A paragraph, with **bold** and `code` as its only markup. */
export const Text = defineComponent({
    name: 'Text',
    description: 'A paragraph of prose.',
    props: z.object({text: z.string()}),
    component: ({props}) => <p>{inline(props.text)}</p>,
})

function inline(text: string) {
    return text.split(/(\*\*[^*]+\*\*|`[^`]+`)/).map((part, i) =>
        part.startsWith('**') ? <strong key={i}>{part.slice(2, -2)}</strong>
        : part.startsWith('`') ? <code key={i}>{part.slice(1, -1)}</code>
        : part)
}
```

  `LibraryTable` reads `SiteContext` with `useContext`. Each row gives:
  - the name, linked to `root + page`;
  - the description;
  - the categories;
  - the targets, joined by ", ";
  - the original as `<a href=url>name version</a>` (language), or "webcpp's own".
- [ ] **Step 4: Write `src/server.tsx`.**
  - Parse the page first with `createParser(library.toJSONSchema())`. When
    `result.meta.unresolved` or `result.meta.validationErrors` is not empty, throw a
    `PageError`:
    - its line is the first line of `source` whose statement names the culprit;
    - its message names the component or the reference.
  - Walk the parsed tree: a `Link` whose `href` starts with `/` throws
    `PageError('... a link inside the site is relative ...')`.
  - Render with `renderToString`, inside the context provider:

```tsx
<Renderer.Root response={source} library={library} isStreaming={false}><Renderer.Content /></Renderer.Root>
```

  - Wrap the result in the shell: doctype, `<html lang="en">`, head (title, description,
    `<link rel="stylesheet" href="${root}assets/site.css">`, the canonical URL), the
    header with the logo and the navigation, the band, `<main>`, and the footer.
  - **The band:**
    - on `preview`: `Preview of main at <code>${commit.slice(0, 7)}</code>, ${date}`;
    - on `release`: `webcpp ${version}`.
  - **The checks run in this order:** the parse (unknown components, unresolved
    references), the links, then the head's limits. A page with several faults reports
    the first, which is what the tests of Step 1 expect.
  - **The SEO head** of the spec's SEO section:
    - the title is the page's `title`, then " - webcpp", at most 60 characters, else a
      `PageError`;
    - the description is 120 to 160 characters, else a `PageError`;
    - the canonical URL and `og:url` come from the channel's base and `context.path`;
    - `og:image` is `cards/<path or "home">.png`, which Task 4 renders;
    - JSON-LD is one `@graph` with `WebSite` (and its `SearchAction` to `search/?q=`),
      `Organization`, `WebPage` and a `BreadcrumbList` built from `context.path`, written
      with `JSON.stringify` and every `<` escaped as `\u003c`.
- [ ] **Step 5: Write `src/client.tsx` and `tools/build.mjs`.**
  - **`src/client.tsx`:**
    - reads `#page-source` and `#site-context`;
    - calls `hydrateRoot(document.getElementById('root'), <the same provider and
      Renderer>)`;
    - a hydration mismatch is logged as an error, which Task 18's console check turns into
      a failure.
  - **`--bundle`:**
    - also bundles `src/client.tsx` to `build/client/site.js` (`platform: 'browser'`,
      `format: 'esm'`, minified, the same `define`);
    - bundles `src/server.tsx` to `build/server.mjs` with esbuild,
    `platform: 'node', format: 'esm', jsx: 'automatic',
    define: {'process.env.NODE_ENV': '"production"'}`, with only `react` and `react-dom`
    external. `@openuidev/react-lang` and `zod` are bundled, so that what Step 7 reads is
    what ships.
  - `--render --pages <dir> --out <dir> --context <file.json>`: renders every
    `pages/**/*.oui` to `<out>/<path>/index.html`. `pages/index.oui` becomes `index.html`,
    and `pages/libraries.oui` becomes `libraries/index.html`.
  - It also copies `assets/` to `<out>/assets/`, and `build/client/site.js` to
    `<out>/assets/site.js`.
  - A `PageError` prints `<file>:<line>: <message>` and exits 1.
- [ ] **Step 6:** Run `npm run build -- --bundle && npm test`. Expected: PASS, 8 tests
  (with Task 1's).
- [ ] **Step 7: Prove the bundle carries no devtools and no CDN.** Add this test, see it
  fail with the define removed, then pass with it back:

```js
test('both bundles are production: no devtools widget, no CDN', () => {
    for (const file of ['../build/server.mjs', '../build/client/site.js']) {
        const bundle = readFileSync(new URL(file, import.meta.url), 'utf8')
        assert.doesNotMatch(bundle, /@openuidev\/devtools/, file)
        assert.doesNotMatch(bundle, /https:\/\/(cdn|unpkg|esm\.sh)/, file)
    }
})
```

  Step 5 bundles react-lang into both bundles, so this test reads the code that ships.
- [ ] **Step 8: Write `assets/site.css`.**
  - Mobile first: the base rules are for 360 px, with `min-width` media queries at 768,
    1280 and 1440.
  - Tokens on `:root`, light and dark themes, and a single accent.
  - This is a provisional theme; Task 8 replaces it with the chosen design.
- [ ] **Step 9: Commit** (site): `"The site's components and renderer: OpenUI Lang pages
  rendered to static HTML, production bundle"`.

## Task 3: The catalogue

**Files:**
- Create (superproject):
  - `tools/site/catalogue.py`;
  - `tools/site/catalogue_test.py`.

**Interfaces:**
- Consumes:
  - every `libs/*/meta/libraries.json`;
  - the lines `<library> <target>` that `b2 -d0 declared-targets` prints.
- Produces:

```
catalogue.py --libs <libs-dir> --declared <file> --out <libraries.json>
```

  It writes a JSON array of `Library` (Task 2's type), sorted by `key`, exit 0. On an
  error it writes nothing, prints `catalogue.py: <file>: <what>` and exits 1.

- [ ] **Step 1: Write the failing tests** `tools/site/catalogue_test.py` (unittest, as
  `tools/doc/*_test.py` are). Each builds a fake `libs/` in a temporary directory:
  - a port (`port-of` set) and an own library (`"port-of": null`) give two entries;
    `port_of` is `null` for the second;
  - targets come from `--declared`, sorted and without duplicates;
  - a library that declares no target has `"targets": []`;
  - a meta without `description` fails, naming the file and `description`;
  - a meta whose `key` differs from its directory's name fails, naming both;
  - the output is byte for byte stable: running twice writes the same file.
- [ ] **Step 2:** Run `python3 tools/site/catalogue_test.py` (the tools tests are standalone scripts: `python3 -m unittest` on them runs 0 tests).
  Expected: FAIL, `No module named catalogue`, or the script is missing.
- [ ] **Step 3: Write `catalogue.py`.** It reads the metas as `tools/doc/libraries.py`
  reads them; reuse its reader if it exposes one, else factor it into a function both
  import. Then:
  - `page = f"libs/{key}/"`;
  - `repository = f"https://github.com/webcpporg/{key}"`;
  - JSON written with `indent=4`, `sort_keys=True`, `ensure_ascii=False`, and a final
    newline.
- [ ] **Step 4:** Run the tests. Expected: PASS, 6 tests.
- [ ] **Step 5:** Run it on the real tree:
  `$B2 -d0 declared-targets > $TMPDIR/declared.txt && python3 tools/site/catalogue.py --libs libs --declared $TMPDIR/declared.txt --out $TMPDIR/libraries.json`.
  Expected: 5 entries. pratt has `emscripten`; trystero has `port_of.version` `0.26.0`.
  Record the targets of each in the ledger.
- [ ] **Step 6: Commit** (superproject): `"tools/site/catalogue.py: libraries.json, the
  site's catalogue, from every library's meta and declared targets"`.

## Task 4: The pages and the indexing files

**Files:**
- Create (site):
  - pages, in `pages/`: `index.oui`, `libraries.oui`, `cpp.oui`, `javascript.oui`,
    `ai.oui`, `examples.oui`, `community.oui`, `search.oui`;
  - `tools/indexing.mjs`;
  - `test/pages_test.mjs`, `test/indexing_test.mjs`.

**Interfaces:**
- Consumes: Task 2's `renderPage` and `tools/build.mjs --render`; Task 3's
  `libraries.json`.
- Produces:

```
node tools/indexing.mjs --site <out> --base <url> --catalogue <libraries.json>
```

  It writes `robots.txt`, `sitemap.xml` and `llms.txt` into `<out>`, and copies the
  catalogue to `<out>/libraries.json`.

- [ ] **Step 1: Write the failing tests.**
  - **`test/pages_test.mjs`** renders every page with a fixed context:
    - each has a `<title>` and a `<meta name="description">`;
    - each links to `libraries/`, `cpp/`, `javascript/` and `ai/` through the header;
    - `index.html` holds "Where C++ and the web meet";
    - `libraries/index.html` holds one row per catalogue entry;
    - `search/index.html` holds a form whose action is `https://www.google.com/search`,
      with a hidden `q` prefix of `site:webcpporg.github.io`;
    - every page carries its OpenUI Lang source, and its body holds no hand-written HTML
      beyond the shell: every element under `#root` comes from a component.
  - **`test/seo_test.mjs`:**
    - for each entry of `seo/queries.json`, every word of the query, short words aside,
      appears in its page's title, `<h1>` or description, case-insensitively;
    - every page of `pages/` has a social card, and a card is 1200 by 630 (read from the
      PNG header);
    - the libraries page's JSON-LD holds an `ItemList` with one `SoftwareSourceCode` per
      catalogue entry, each with `programmingLanguage` "C++", `codeRepository`, `license`
      and `runtimePlatform` from its targets.
  - **`test/indexing_test.mjs`**, over a fake `<out>` with three `index.html`:
    - `robots.txt` is exactly:

      ```
      User-agent: *
      Allow: /

      User-agent: Google-Extended
      Allow: /

      Sitemap: <base>sitemap.xml
      ```

    - `sitemap.xml` lists the three pages as `<base><path>`, sorted;
    - `llms.txt` starts with `# webcpp`, then has a line `- [<name>](<base>libs/<key>/):
      <description>` per library and one line per page;
    - a `--base` without a final `/` fails.
- [ ] **Step 2:** Run `npm test`. Expected: FAIL; the pages and `indexing.mjs` do not exist.
- [ ] **Step 3: Write the pages.** Their content follows the spec's section 5 and decision 8.
  - **`index.oui`:**
    - the Hero "Where C++ and the web meet";
    - `Targets()`: browser (emscripten), server as a WASI container (wasmtime, clouds),
      native;
    - `Doors()`;
    - a section "Examples", pointing to `examples/`.
  - **`javascript.oui`:** what exists today (pratt and trystero build for emscripten; the
    chat page), and a `Roadmap` of what is coming:
    - trystero's JavaScript API and Trystero's examples;
    - JavaScript functions exported to C++;
    - measured sizes;
    - the same code on a server.
  - **`ai.oui`:** `llms.txt`, `libraries.json`, and the roadmap of integrations.
  - **`examples.oui`:** in increment 1, the gallery says that the first example comes with
    the visualizer, and lists the coming ones.
  - **`community.oui`:** links to CONTRIBUTING.md and AGENTS.md on GitHub.
- [ ] **Step 4: Write `tools/indexing.mjs`, `seo/queries.json` and `tools/cards.mjs`.**
  - `seo/queries.json` starts with the spec's first set of queries, each with its page.
  - `tools/cards.mjs` renders `cards/<page>.png` from `assets/card.svg` (the page's title
    and the logo, set in the template's text) with `@resvg/resvg-js`, called by the
    `build` script's `--render` step.
  - `sitemap.xml` gives each URL its `<lastmod>`, the date of the commit that last changed
    its source.
- [ ] **Step 5:** Run `npm test`. Expected: PASS.
- [ ] **Step 6: Commit** (site): `"The pages: home, libraries, the three doors, examples,
  community and search; robots.txt, sitemap.xml and llms.txt"`.

## Task 5: `b2 site` and the assembly

**Files:**
- Create (superproject):
  - `tools/site/site.jam`, the target's rules;
  - `tools/site/site_test.py`, which runs `b2 site` on a fixture.
- Modify (superproject):
  - `Jamroot`: `alias site`;
  - `tools/ci/assemble.py` and `tools/ci/assemble_test.py`;
  - `tools/doc/doc.jam`: the site layout's index links.
- Modify (site): `Jamfile` (the `pages` target).
- Create (site):
  - `tools/serve.mjs`, the `serve` script: node's `http` module over a directory on
    127.0.0.1, with the right types for `.wasm`, `.webmanifest` and `.mjs`;
  - `tools/dev.mjs`, the `dev` script:
    - builds the site's own pages with `test/fixtures/catalogue.json` and a fixed context
      into `build/dev/`;
    - serves them, and rebuilds on a change under `pages/`, `src/` or `assets/`;
    - takes the examples' wasm from `bin/site/` when the superproject built it, else from
      the published preview, downloaded once into `build/dev/` and checked against the
      preview's `precache.json`. This is the one network access outside `npm ci`, and it
      happens only in `dev`, never in a build or a test;
  - `test/readme_test.mjs`.

**Interfaces:**
- Consumes:
  - Task 2's `tools/build.mjs --render`;
  - Task 3's `catalogue.py`;
  - Task 4's `indexing.mjs`;
  - `b2 doc` (built in the `site` layout).
- Produces:
  - `b2 site` writes `bin/site/`;
  - `assemble.py --pages DIR --docs ROOT --report DIR|- --chrome FILE --catalogue FILE --base URL --out DIR`.

  The layout of `bin/site/`:

```
index.html, libraries/, cpp/, javascript/, ai/, examples/, community/, search/   the site's pages
assets/                     the site's styles and, later, scripts
docs/index.html             the superproject's index page (doc/index.adoc), its links ../libs/<name>/
libs/<name>/                each library's page, as today
report/                     the test matrix, or one page saying no results were given
robots.txt, sitemap.xml, llms.txt, libraries.json
```

- [ ] **Step 1: Write the failing tests.**
  - **`test/readme_test.mjs`** (site):
    - every `npm run <script>` and `npm <command>` the README shows is a script of
      `package.json` or an npm command;
    - every `b2 <target>` it shows is one of `site`, `site-serve`, `/webcpp/site//test`,
      `/webcpp/site//browser-test`;
    - the section "For C++ developers" contains no `npm`, no `package.json` and no
      `esbuild`;
    - the section "For JavaScript developers" has both paths;
    - `dev` uses a local wasm when one is given, and falls back to the preview's,
      checking its hash; tested with a fake fetch, without the network.
  - **`assemble_test.py`**, new cases:
    - the site's pages land at the root and the docs index at `docs/`;
    - `--report -` writes `report/index.html` with "No test results were given to this
      build";
    - `--chrome` inserts the fragment after `<body>` of every page under `docs/` and
      `libs/`, and a `<link rel="stylesheet">` to `assets/site.css`, relative from each
      page, before `</head>`;
    - `--catalogue` gives each library's page, before `</head>`:
      - the SEO head: the title "`<name>`: `<description's first clause>` - webcpp" (at
        most 60 characters), the description, the canonical URL, Open Graph, the Twitter
        card;
      - JSON-LD with `SoftwareSourceCode` (`isBasedOn` the original of a port) and
        `TechArticle`.

      A library whose description cannot make a title of 60 characters fails, naming it.
    - a link to `/x` still fails;
    - the home page must link to `libraries/`. The old rule "index links every library"
      becomes "`libraries/index.html` links every library".
  - **`site_test.py`** runs, in a scratch copy of the superproject, `b2 site` twice, with
    `-sWEBCPP_SITE_DOCS` pointing at a fixture of pages. A page is deleted from the site's
    `pages/` between the runs; the second `bin/site/` no longer holds it (Review Focus 3).
    It also covers the channel options:
    - `-sWEBCPP_SITE=release` without `-sWEBCPP_SITE_VERSION` fails, naming the option;
    - `-sWEBCPP_SITE=draft` fails, naming the two channels.
- [ ] **Step 2:** Run them. Expected: FAIL.
- [ ] **Step 3: The site layout of the docs index.** In `tools/doc/doc.jam` change
  `.index-links.site = library-pages=libs/ library-page= ;` to
  `.index-links.site = library-pages=../libs/ library-page= ;`, and say in its comment
  that the site serves the index at `docs/`.
- [ ] **Step 4: Teach `doc.jam` the site target.** The layout is `site` when
  `-sWEBCPP_INDEX=site` is given, or when `site` is among the targets on b2's command
  line (`[ modules.peek : ARGV ]`, the words that do not start with `-`), so that
  `b2 site` alone builds the pages it needs.
- [ ] **Step 5: Write the site's `pages` target** in `site/Jamfile`: a `make` of
  `pages.stamp` whose action runs, through `site.npm` (Step 6):
  1. `npm run build -- --bundle`;
  2. `npm run build -- --render --pages pages --out <build-dir>/pages --context
     <build-dir>/context.json`.
  `context.json` is written by `tools/site/site.jam` from the channel, `git -C .
  rev-parse HEAD`, the commit's date (`git log -1 --format=%cs`), the version, and the
  catalogue. Every node action quotes its paths and uses backquotes, never `$(...)`, as
  b2's facts require.
- [ ] **Step 6: Write `tools/site/site.jam` and the `Jamroot` aliases.**
  - **`rule npm ( script : arguments * )`** (`site.npm`): an action that runs
    `npm run <script> -- <arguments>` in the site's directory, after `node-modules`, with
    node from the toolchain the doc build already finds.
  - **`b2 site-serve`:**
    - depends on `site`, and ECHOs `Serving bin/site at http://127.0.0.1:8765/` when the
      Jamfile loads, since b2 shows an action's output only when it ends;
    - then runs `npm run serve -- bin/site 8765`.
  - **The `site` notfile:**
  1. checks the channel options;
  2. writes `bin/site-work/declared.txt` (`b2 -d0 declared-targets` is a rule of this
     Jamroot: call `webcpp.declared` directly, as `declared-targets` does, instead of a
     nested b2);
  3. runs `catalogue.py`;
  4. depends on `/webcpp/site//pages` and, unless `-sWEBCPP_SITE_DOCS` is given, on
     `doc`;
  5. removes `bin/site` and runs `assemble.py` into it;
  6. runs `indexing.mjs` with the channel's base: `https://webcpporg.github.io/` for
     preview, `https://webcpp.org/` for release.
  The chrome fragment comes from `renderFragment('docs-header', context)`, written by the
  `pages` action to `<build-dir>/docs-header.html`.
- [ ] **Step 7:** Run the tests. Expected: PASS.
- [ ] **Step 8: Build it for real:** `$B2 -a --build-dir=bin/b-site site-serve`, and open
  `http://127.0.0.1:8765/` in Chrome.
  - Look at every page in light and dark, at 360, 768, 1280 and 1440 px.
  - Write down every visual fault and fix it before the commit (the global rule on pixel
    perfection).
  - Expected: `assemble.py: wrote bin/site, M pages, 5 libraries, every link resolving`.
    M is the Task 0 baseline plus the site's eight pages.
- [ ] **Step 9: Commit**, the superproject (`Jamroot`, `tools/site/*`, `tools/ci/assemble*`,
  `tools/doc/doc.jam`) and the site (`Jamfile`):
  - superproject: `"b2 site and b2 site-serve: the whole site into bin/site/, the site's npm
    scripts through site.npm, the site's pages at the root,
    the docs index at docs/, every library's page under the site's header, every link
    checked"`;
  - site: `"The pages target, which b2 site renders"`.

## Task 6: The site's own tests in b2

**Files:**
- Modify (site): `Jamfile` (target `test`).
- Modify (superproject): `tools/ci/matrix.py` and `tools/ci/matrix_test.py`, so that the
  fast level runs `/webcpp/site//test`.

- [ ] **Step 1: Write the failing tests.**
  - In `matrix_test.py`, the fast level's lanes include `site-test`.
  - A fixture page with an unclosed `<p>` inside a `<ul>` fails `valid-html`, naming the
    file.

  Run them. Expected: FAIL.
- [ ] **Step 2: Declare the target.** `site/Jamfile` declares `notfile test`, which runs
  `npm test` and `npm run typecheck` in the site's directory, depending on
  `node-modules`. It also declares `notfile valid-html`, which runs
  `npx html-validate --config .htmlvalidate.json` over every `bin/site/**/*.html`.
  - `valid-html` is a dependency of `b2 site`'s last step, so a page that is not valid
    HTML fails the build, naming the file and the rule.
  - `.htmlvalidate.json` extends `html-validate:recommended`. Every rule turned off there
    carries a comment saying why.
  - The libraries' Asciidoctor pages are validated too. A finding in them is fixed in
    `tools/doc/` or in the page, never silenced.
- [ ] **Step 3: Add the lane** `site-test` to `matrix.py`, running
  `b2 -a /webcpp/site//test`, at every level.
- [ ] **Step 4:** Run both test suites. Expected: PASS.
- [ ] **Step 5: Commit** both repositories: `"The site's tests as a b2 target and a CI
  lane"`.

## Task 7: The preview band and the release channel

**Files:**
- Modify (site): `src/server.tsx`; `test/render_test.mjs`.

- [ ] **Step 1: Write the failing tests.**
  - On `release`, the band reads `webcpp <version>`, with no commit.
  - On `preview` with a `version` set, the build fails: preview carries no version.
  - The canonical URL is `https://webcpporg.github.io/<path>` on preview and
    `https://webcpp.org/<path>` on release.
- [ ] **Step 2:** Run. Expected: FAIL on the canonical URL and on the version rule.
- [ ] **Step 3:** Implement.
- [ ] **Step 4:** Run. Expected: PASS.
- [ ] **Step 5:** Run `$B2 -a --build-dir=bin/b-site site -sWEBCPP_SITE=release
  -sWEBCPP_SITE_VERSION=0.0.0-test`. Expected: the band reads `webcpp 0.0.0-test`, and
  `sitemap.xml` uses `https://webcpp.org/`.
- [ ] **Step 6: Commit** (site): `"The release channel: its band, its canonical URLs"`.

## Task 8: The logo and the home, for the owner (gate G3)

**Files:**
- Create (site):
  - `design/logo-a.svg`, `design/logo-b.svg`, `design/logo-c.svg`: three concepts;
  - `design/home-mockup.html`: the home with each concept;
  - after G3: `assets/logo.svg` and the final `assets/site.css`.

**The brief:**
- Apple's restraint: large type, much space, one accent, motion only where it explains.
- The headline "Where C++ and the web meet".
- The logo joins C++'s `++` with the web's sign. Three directions to show:
  - **a.** The two plus signs as the meridians of a globe;
  - **b.** A `{` and a `}` meeting a `<` and a `>`;
  - **c.** Two linked rings, a C and a W.
- Each concept works at 16 px (favicon) and as a 512 px app icon, in one colour and in
  two, light and dark.
- The mockup is mobile first: shown at 360 px and at 1280 px side by side.

- [ ] **Step 1:** Draw the three SVGs.
  - Hand-written SVG: viewBox 0 0 64 64, no embedded raster.
  - Each with a `<title>`.
- [ ] **Step 2:** Write `design/home-mockup.html`, self-contained (no external font, no
  script but a toggle between the three logos and between light and dark).
- [ ] **Step 3: The controller publishes the mockup** as a private artifact (the
  Artifact tool) and asks the owner to choose (G3). It does not wait idle: Tasks 9 to 11
  continue with concept a in place.
- [ ] **Step 4: After the owner's choice:**
  - copy the chosen SVG to `assets/logo.svg`;
  - fold the mockup's styles into `assets/site.css`;
  - make the favicon from the same SVG;
  - re-run `npm test` and Task 5's visual pass.
- [ ] **Step 5: Commit** (site): `"The logo and the theme, as the owner chose"`.

## Task 9: Publication and the redirect (gates G1 and G2)

**Files:**
- Create (superproject):
  - `tools/site/redirect.py`;
  - `tools/site/redirect_test.py`.
- Modify (superproject): `.github/workflows/ci.yml`.
- Create (site): `.github/workflows/site.yml`, the site repository's checks on pull
  requests.

**Interfaces:**
- Produces:

```
redirect.py --to https://webcpporg.github.io/ --prefix /webcpp/ --out DIR
```

  It writes `index.html` and `404.html`. Each:
  - holds `<meta http-equiv="refresh" content="0; url=https://webcpporg.github.io/">` and
    `<link rel="canonical">`;
  - holds a script that replaces the location with the same path without the prefix, its
    query and its fragment kept.

- [ ] **Step 1: Write the failing tests:**
  - both files exist;
  - the script maps `/webcpp/libs/pratt/#pratt-limits` to
    `https://webcpporg.github.io/libs/pratt/#pratt-limits`. The page's script defines
    `function target(location)` between the comments `/* target: begin */` and
    `/* target: end */`. The test extracts that text and runs it with `node -e`, on
    objects shaped like `window.location`: `/webcpp/` maps to the root, and
    `/webcpp/report/?lane=x` keeps its query;
  - `--to` without `https://` fails.
- [ ] **Step 2:** Run. Expected: FAIL.
- [ ] **Step 3: Write `redirect.py`.**
- [ ] **Step 4:** Run. Expected: PASS.
- [ ] **Step 5: Change `ci.yml`.**
  - **The `site` job:**
    - checks out with `submodules: true`, the site included;
    - sets up node 26.7.0;
    - runs `b2 site -sWEBCPP_SITE_DOCS=bin/pages -sWEBCPP_SITE_REPORT=bin/report` in place
      of `assemble.py`;
    - uploads `bin/site` as the artifact `site`.
  - **A new job, `publish`, on a push to `main` only:**
    - downloads `site`;
    - writes the deploy key from `secrets.SITE_DEPLOY_KEY` to a file of mode 600 that no
      step prints;
    - commits the tree to the branch `gh-pages` of
      `git@github.com:webcpporg/webcpporg.github.io.git` as one commit on an orphan branch,
      with the plan's identity and the message `"The preview of main at <sha>"`;
    - force-pushes that branch alone (`gh-pages` is build output, replaced whole).
    - A `.nojekyll` file goes in the tree.
  - **The existing `pages` job** deploys `redirect.py`'s output in place of the site.
  - Run `python3 tools/ci/actions_test.py` and any workflow test the tree has, each as `python3 <path>`.
    Expected: PASS.
- [ ] **Step 6: Write the site repository's `.github/workflows/site.yml`:**
  - on pull requests and pushes;
  - `npm ci`, `npm test`, `npm run typecheck`, and
    `node tools/build.mjs --bundle --render ...` over a fixture catalogue in `test/`;
  - no publication.
- [ ] **Step 7: Commit** both repositories:
  - superproject: `"CI: b2 site builds the site, published to webcpporg.github.io's
    gh-pages; the old address redirects to the root"`;
  - site: `"The site's checks on pull requests"`.
- [ ] **Step 8 (controller, G1 then G2):**
  - ask the owner;
  - then create the repository, its Pages from `gh-pages`, the deploy key and the secret;
  - push the site's `main`.

## Task 10: The submodule

**Files:**
- Modify (superproject): `.gitmodules`, `site` (the gitlink), `.git/info/exclude` (remove
  `/site/`).

- [ ] **Step 1:** Remove `/site/` from `.git/info/exclude`.
- [ ] **Step 2:** In the superproject:
  `git submodule add git@github.com:webcpporg/webcpporg.github.io.git site`, with the
  existing directory kept. The site repository is already pushed at G2: move the local
  repository aside, add the submodule, check that its HEAD is the same commit, and remove
  the copy.
- [ ] **Step 3:** Run `$B2 -a --build-dir=bin/b-site site`. Expected: as in Task 5,
  Step 8.
- [ ] **Step 4: Commit:** `"site/: the site's repository, webcpporg/webcpporg.github.io, as
  a submodule"`.

## Task 11: The published addresses, and the rulebook

**Amended 2026-10-11 (owner): the preview is not indexed.** Besides the addresses, the
preview channel (the build's default; Task 7's release channel is the other) writes
`<meta name="robots" content="noindex">` in every page's head, the library pages and the
report included; its robots.txt names no sitemap and the preview writes no sitemap.xml;
the release channel keeps both and has no noindex. Tests, red first: a preview build's every
HTML page carries the meta, a release build's none; the preview has no sitemap.xml and its
robots.txt has no Sitemap line; the release build has both. llms.txt and libraries.json
are written on both channels.

**Files:**
- Modify:
  - the superproject: `README.md`, `AGENTS.md` (chapters 1, 8, 9 and 13), `doc/index.adoc`;
  - each library: `README.md` and `doc/testing.adoc`, in xactor, xstate, pratt, wasi and
    trystero;
  - `tools/doc/extensions_test.py`, only if its fixtures must follow (they are sample text
    about bare links; leave them, and say so in the report).

- [ ] **Step 1:** Replace `https://webcpporg.github.io/webcpp/report/` with
  `https://webcpporg.github.io/report/` and `https://webcpporg.github.io/webcpp/` with
  `https://webcpporg.github.io/` in each file of Task 0, Step 3.
- [ ] **Step 2: AGENTS.md:**
  - chapter 9 gains "The site": `b2 site`, the channels, the options, the layout of
    `bin/site/`, and where the site's source lives;
  - chapter 1's layout table gains `site/`;
  - chapter 13 marks increment 1 done and lists increments 2 to 8.
- [ ] **Step 3:** Run every library's doc-check with `$B2 -a --build-dir=bin/b-doc doc
  -sWEBCPP_INDEX=site`, and `$B2 -a --build-dir=bin/b-site site`. Expected: green.
- [ ] **Step 4: Commit** each library, then the superproject with the pointers:
  - libraries: `"The test report's address at the organisation's root"`;
  - superproject: `"The site's addresses at the root; AGENTS.md: the site"`.

---

## Task 12: xstate on emscripten (increment 2)

**Files:**
- Modify (xstate):
  - `test/Jamfile` and `example/Jamfile`: `webcpp.targets native emscripten wasip2 wasip3`;
  - the headers a failure points to;
  - `doc/*.adoc` where the targets are stated (the counts are computed).
- Modify (superproject): `README.md`'s table row for xstate.

- [ ] **Step 1: Write the failing test.** Add `emscripten` to both Jamfiles' targets and
  run `$B2 -a --build-dir=bin/b-em toolset=emscripten libs/xstate/test libs/xstate/example`.
  Expected: FAIL. Record every failing program and its first error in the ledger.
- [ ] **Step 2: Sort the failures.**
  - A failure of the core is a defect to fix in the core: the machine layer, under
    `include/webcpp/xstate/*.hpp`, which includes no xactor header. It is reproduced by
    the failing program itself, then fixed.
  - A failure of the actor layer (`actors/`, on xactor) is fixed when the cause is a
    portable one.
  - When the cause is xactor's emscripten support, those programs get explicit targets
    `native wasip2 wasip3` in their declarations, with a comment naming the cause. The
    ledger then carries a finding for the allocators-and-async review (AGENTS.md chapter
    13).
- [ ] **Step 3:** Fix, one failure at a time, re-running the failing program after each.
- [ ] **Step 4:** Run the whole lane again. Expected: PASS, every program declared for
  emscripten.
- [ ] **Step 5:** Run xstate's native, wasip2 and wasip3 lanes, its doc, the lint and the
  oracle lane. Expected: PASS; nothing else changed.
- [ ] **Step 6: Commit** xstate, then the superproject pointer and README:
  - xstate: `"xstate on emscripten: the machine layer's tests and examples in the browser's
    target"`;
  - superproject: `"libs/xstate: on emscripten"`.

## Task 13: The site's wasm, running the player's machine

**Files:**
- Create (site):
  - `app/site_app.cpp`, `app/site_app.hpp`: the API, testable natively;
  - `app/bindings.cpp`: embind;
  - `app/Jamfile`;
  - `app/test/site_app_test.cpp`: native, lightweight_test;
  - `machines/player.json`;
  - `src/machine.ts`: the loader and the React hook;
  - `test/machine_test.mjs`.

**Interfaces:**
- Produces (C++, `namespace webcpp_site`):

```cpp
/** One machine of the site, created from XState's JSON config, stepped by events. */
class machine_host {
public:
    /** The machine of config, in its initial state, or the error that refuses the config. */
    static boost::system::result<machine_host> create(std::string_view config_json);
    /** Sends event (XState's event JSON, {"type": ...}) and returns the snapshot's JSON. */
    boost::system::result<std::string> send(std::string_view event_json);
    /** The current snapshot as JSON: {"value": ..., "context": ..., "status": ...}. */
    std::string snapshot() const;
};
```

- Produces (embind, module `SiteApp`): `class MachineHost` with `static create(config:
  string): MachineHost` (throws a JS `Error` with the message when the config is refused),
  `send(event: string): string`, `snapshot(): string`, `delete()`.
- Produces (TypeScript): `loadSiteApp(url: string): Promise<SiteApp>` and
  `useMachine(config: object): [snapshot, send]`.
- `machines/player.json`, XState's JSON:
  - states: `idle`, `running`, `paused`, `stepping`, `done`;
  - events: `PLAY`, `PAUSE`, `STEP`, `RESTART`, `TICK`, `END`, `SPEED` (`{speed: 0.5|1|2}`);
  - context: `{speed: 1, played: 0}`.

| From | Event | To | Context |
| --- | --- | --- | --- |
| idle | PLAY | running | played 0 |
| idle | STEP | stepping | |
| running | PAUSE | paused | |
| running | TICK | running | played + 1 |
| running | END | done | |
| paused | PLAY | running | |
| paused | STEP | stepping | |
| stepping | TICK | paused | played + 1 |
| stepping | END | done | |
| done | RESTART | idle | played 0 |
| any | SPEED | same | speed = event.speed |

- [ ] **Step 1: Write the failing native test** `app/test/site_app_test.cpp`:
  - create from `player.json` (read from `machines/`, its path passed as an argument by
    the Jamfile); the snapshot's value is `"idle"`;
  - `PLAY`, `TICK`, `TICK`, `PAUSE`: value `"paused"`, `played` 2;
  - `SPEED` with 2 in `paused` keeps `"paused"`, with `speed` 2;
  - `END` in `running` gives `"done"`;
  - `RESTART` gives `"idle"` with `played` 0;
  - a config that is not JSON is refused, `create` returning the error;
  - an event that is not JSON makes `send` return an error, and the state does not move.
- [ ] **Step 2:** Run `$B2 -a /webcpp/site/app//site_app_test`. Expected: FAIL (no
  sources).
- [ ] **Step 3: Write `site_app.hpp` and `.cpp`** on webcpp::xstate's machine layer:
  - the config is loaded as xstate loads XState's JSON;
  - assignments of `played` and `speed` are implementations given by name (xstate's
    `implementations.hpp`).
- [ ] **Step 4:** Run. Expected: PASS.
- [ ] **Step 5: Write `bindings.cpp` and the emscripten build.** `app/Jamfile` declares
  `site-app` with:
  - `<linkflags>-lembind`;
  - `<linkflags>-sMODULARIZE -sEXPORT_ES6 -sEXPORT_NAME=SiteApp -sENVIRONMENT=web,node`;
  - `<linkflags>-sALLOW_MEMORY_GROWTH`.

  The site's `pages` target depends on `app//site-app` built with emscripten:
  - **First,** prove in a scratch Jamfile whether a target reference with a property,
    `app//site-app/<toolset>emscripten`, builds with that toolset while `b2 site` runs
    with the default toolset. Record the answer in the ledger.
  - **If it does not,** `b2 site` requires `toolset=emscripten` for the wasm part: the
    site target then runs a second b2 for `app//site-app` with `toolset=emscripten` and
    its own `--build-dir`. Record that ruling.
- [ ] **Step 6: Write the failing node test** `test/machine_test.mjs`. It loads the built
  `site-app.mjs` (path from an environment-free argument: the test reads
  `build/app/site-app.mjs`, which the b2 target copies there) and plays the same sequence
  as Step 1.
  - It also runs the same sequence on `xstate` 5.33.2's `createActor(createMachine(json))`
    and asserts that the two values and contexts agree at each step. XState is the oracle,
    in the test only.
- [ ] **Step 7:** Run. Expected: FAIL before the copy exists, PASS after.
- [ ] **Step 8: Write `src/machine.ts`.**
  - `useMachine` holds one `MachineHost`, re-renders on each `send`, and deletes the host
    on unmount.
  - A failed `loadSiteApp` gives the hook the state `{error: message}`, which the
    visualizer shows as text (Review Focus 2).
- [ ] **Step 9: Commit** (site): `"The site's wasm: its machines on webcpp::xstate, the
  player's machine, XState as the test's oracle"`.

## Task 14: The PWA (increment 2)

**Files:**
- Create (site):
  - `machines/install.json`, the install flow;
  - `src/sw.ts`, bundled to `sw.js` at the site's root;
  - `src/install.tsx`: the `InstallPrompt` component and the flow's hook;
  - `tools/precache.mjs`: `precache.json`;
  - `assets/manifest.webmanifest`;
  - tests: `test/sw_test.mjs`, `test/install_test.mjs`, `test/precache_test.mjs`;
  - `app/test/install_test.cpp`.
- Modify (site):
  - `src/server.tsx`: the manifest link and the theme colour (the Apple touch icon and the
    favicon links exist since Task 8);
  - `tools/icons.mjs` (created by Task 8, site b698a9c, with `tools/drawing.mjs`): add the
    icons the manifest names (192 and 512, any and maskable) if they are not already among
    the accent-ground icons it writes; it keeps rendering from `assets/logo.svg`;
  - `tools/build.mjs`: the `--pwa` step;
  - `app/site_app.cpp`: nothing; the machine runs on `machine_host` as it is.
- Modify (superproject): `tools/site/site.jam`, whose last step runs
  `npm run build -- --pwa bin/site` after `indexing.mjs`, so that `precache.json` lists
  the final files.

**Interfaces:**
- Consumes: Task 13's `machine_host`, `loadSiteApp` and `useMachine`.
- Produces: `precache.json`:

```json
{
    "version": "<sha256 of the list>",
    "total": 1234567,
    "files": [
        {"path": "index.html", "sha256": "...", "size": 4321}
    ]
}
```

  `sw.js` registers from the shell with scope `./`. `InstallPrompt()` is an OpenUI
  component the shell's header holds on every page.
- `machines/install.json`, XState's JSON:

| From | Event | To | Effect |
| --- | --- | --- | --- |
| checking | COMPLETE | ready | |
| checking | MISSING (`{bytes}`) | asking, or declined when this version was declined | |
| asking | ACCEPT | installing | the service worker caches the missing files |
| asking | DECLINE | declined | remember the version |
| installing | PROGRESS (`{bytes}`) | installing | context `done` += bytes |
| installing | INSTALLED | ready | `navigator.storage.persist()` |
| installing | FAILED (`{reason}`) | asking | the reason shown |
| ready | NEW_VERSION | updating | the changed files fetched in the background |
| updating | UPDATED | ready | offer a reload |

- [ ] **Step 1: Write the failing tests.**
  - **`app/test/install_test.cpp`**, native, on `machine_host`: every row of the table,
    and `MISSING` with a declined version going to `declined`.
  - **`test/install_test.mjs`:**
    - the same sequences on the site's wasm and on XState 5.33.2, agreeing at each step;
    - the hook with a `localStorage` that throws declines for the session only, and asks
      again on the next load (Review Focus 4).
  - **`test/precache_test.mjs`**, over a fixture directory:
    - every file is listed with its SHA-256 and size, sorted by path;
    - `precache.json` and `sw.js` do not list themselves;
    - `total` is the sum;
    - `version` changes when one byte of one file changes, and only that file's entry
      changes.
  - **`test/sw_test.mjs`**, the service worker's handlers against a fake `caches` and
    `fetch`:
    - a precached file is served from the cache without a fetch;
    - a navigation with the network failing gets the cached page;
    - `install` for a new version fetches only the files whose `sha256` differs from the
      cached list's (Review Focus 2);
    - a request to another origin is never cached.
  - **`tools/icons.mjs`:** the test renders the logo to 192 and 512 px PNGs and to a
    180 px Apple touch icon, and checks their sizes from the PNG headers.
- [ ] **Step 2:** Run `npm test` and `$B2 -a /webcpp/site/app//install_test`.
  Expected: FAIL.
- [ ] **Step 3: Implement.**
  - **`sw.ts`:** plain service worker code, no library.
    - `install` reads `precache.json` and caches the missing or changed files by hash,
      posting `PROGRESS` to the page.
    - `activate` removes the entries no longer listed.
    - `fetch` follows the strategies of the spec's PWA section.
  - **The prompt** says, in the reader's terms:
    - that the site can be installed for instant and offline use;
    - the size to download, from `total`;
    - two buttons, Install and Not now.
  - **After `ready`:**
    - where `beforeinstallprompt` fired, "Install the app";
    - on iOS Safari, which has no prompt, how to add the site to the home screen.
- [ ] **Step 4:** Run. Expected: PASS.
- [ ] **Step 5:** Run `$B2 -a --build-dir=bin/b-site site-serve`, and check in Chrome:
  - on a phone width with device emulation:
    - the first visit asks;
    - Install caches everything (DevTools, Application, Cache Storage);
    - offline reload works;
    - the app installs from the browser's menu;
  - on the desktop width: the same.

  Fix every visual fault of the prompt at each width before the commit.
- [ ] **Step 6: Commit** (site) and the superproject's `site.jam`:
  - site: `"The PWA: the manifest, the icons, the service worker of the site's own, and the
    install flow on webcpp::xstate"`;
  - superproject: `"b2 site: the PWA's precache list, written last"`.

## Task 15: The trace and the player's sound

**Files:**
- Create (site):
  - `include/webcpp_site/trace.hpp`;
  - `app/test/trace_test.cpp`;
  - `src/trace.ts`: the loader's wrapping and the event type;
  - `src/sound.ts`;
  - `test/trace_test.mjs`, `test/sound_test.mjs`.

**Interfaces:**
- Produces (C++):

```cpp
// Inside the site's build (WEBCPP_SITE_TRACE defined), the scope sends cpp_enter
// with its name, file, line and arguments as JSON, and cpp_exit when it ends;
// elsewhere it is nothing.
#define WEBCPP_SITE_TRACE_SCOPE(name, ...) /* see trace.hpp */
```

- Produces (TypeScript):

```ts
export type TraceEvent =
    | {seq: number; kind: 'js_to_wasm' | 'wasm_to_js'; name: string; args: unknown[]}
    | {seq: number; kind: 'return'; name: string; result: unknown}
    | {seq: number; kind: 'cpp_enter'; name: string; file: string; line: number; args: unknown[]}
    | {seq: number; kind: 'cpp_exit'; name: string}
    | {seq: number; kind: 'log'; text: string}
export function traced<M extends object>(module: M, imports: Record<string, Function>, sink: (e: TraceEvent) => void): M
export function createSound(): {play(kind: TraceEvent['kind']): void; muted: boolean; setMuted(m: boolean): void}
```

- [ ] **Step 1: Write the failing tests.**
  - **`trace_test.cpp`**, native, without `WEBCPP_SITE_TRACE`: a function with two trace
    scopes compiles, and `sizeof` of the scope guard's type is 1, or the macro expands to
    nothing: `static_assert(std::is_same_v<decltype(webcpp_site::detail::scope_t), void>)`
    on the disabled path.
  - **`trace_test.mjs`:**
    - `traced` over a fake module `{add: (a, b) => a + b}` and an import `{log(t) {}}`
      records `js_to_wasm add [1,2]`, then `return add 3`, in order, with increasing
      `seq`;
    - an export that throws records `return` with `result: {error: message}` and
      rethrows.
  - **`sound_test.mjs`:**
    - with a global `AudioContext` that throws on construction, `play` does nothing and
      does not throw;
    - with `localStorage` whose getter throws, `createSound()` starts unmuted for the
      session and `setMuted` works without storage (Review Focus 4);
    - with `matchMedia('(prefers-reduced-motion: reduce)')` true, `play` still sounds:
      sound and motion are separate preferences.
- [ ] **Step 2:** Run. Expected: FAIL.
- [ ] **Step 3: Implement.**
  - `trace.hpp`: under `WEBCPP_SITE_TRACE`, an `EM_JS` import
    `webcpp_site_trace(kind, name, file, line, args_json)`, and an RAII guard. The
    arguments are JSON through Boost.JSON's `value_from`.
  - `sound.ts`: Web Audio oscillators, one short envelope per kind, frequencies in a table
    in the file. The `AudioContext` is created on the first `play` after a user gesture,
    in `try`/`catch`.
- [ ] **Step 4:** Run. Expected: PASS.
- [ ] **Step 5: Commit** (site): `"The trace, C++ scopes and JavaScript crossings, and the
  player's sound"`.

## Task 16: The visualizer

**Files:**
- Create (site):
  - `src/components/visualizer.tsx`, with `src/visualizer/{js_pane,bridge_pane,cpp_pane,log_pane,controls}.tsx`;
  - `test/visualizer_test.mjs`;
  - `test/fixtures/calculator.trace.json`.
- Modify (site): nothing outside the visualizer: the client of Task 2 hydrates it with
  the rest of the page.

**Interfaces:**
- Produces: the OpenUI component `Visualizer(example, js_source, cpp_source, options)`.
  - Its prerender holds the two code panes, highlighted, for the first paint.
  - Hydrated, it loads the example's wasm and plays.
  - On a phone, one pane shows at a time, with tabs; from 1280 px, the four stand side by
    side.
- Behaviour:
  - the player's state comes from `useMachine(player.json)`;
  - `TICK` is sent by a timer at `1000 / (4 * speed)` ms in `running`, and by the step
    button in `stepping`;
  - each tick shows the next trace event: the bridge tile for crossings, the line in the
    JS or C++ pane, a line in the log, and a sound;
  - with `prefers-reduced-motion`, tiles change state without transition.

- [ ] **Step 1: Write the failing tests** `test/visualizer_test.mjs`:
  - the server render of `Visualizer` holds both sources as text and the tabs of the
    phone layout;
  - a pure function `frame(trace, n)` (exported from `src/visualizer/frame.ts`) gives, for
    `n` events played:
    - the highlighted JS line;
    - the highlighted C++ line;
    - the tiles' states;
    - the log lines.

    Assert it on the fixture trace at `n` = 0, 1, 4 and `trace.length`;
  - every control of the player is a `<button>` with an `aria-label`, and the visualizer
    has `role="region"` with the example's name, so that the player works from the
    keyboard and a screen reader names it.
- [ ] **Step 2:** Run. Expected: FAIL.
- [ ] **Step 3: Implement.** The panes are plain React over `frame`.
- [ ] **Step 4:** Run. Expected: PASS.
- [ ] **Step 5: Commit** (site): `"The bridge visualizer: four panes over a trace, played by
  the player's machine"`.

## Task 17: The calculator example

**Files:**
- Create (site):
  - `examples/calculator/calculator.cpp`: pratt's calculator, embind, trace scopes;
  - `examples/calculator/calculator.js`: the page's script;
  - `examples/calculator/Jamfile`;
  - `pages/examples/calculator.oui`: the live page;
  - `pages/examples/calculator/explained.oui`;
  - `test/calculator_test.mjs`.
- Modify (site): `pages/examples.oui`, the gallery.

**Interfaces:**
- Consumes:
  - pratt's calculator, as `libs/pratt/example/calculator_repl.cpp` uses it;
  - `WEBCPP_SITE_TRACE_SCOPE`;
  - `traced`.
- Produces (embind, module `Calculator`): `evaluate(text: string): {ok: boolean, value:
  string, error: string}`.

- [ ] **Step 1: Write the failing test** `test/calculator_test.mjs`, on the built
  `build/examples/calculator.mjs`:
  - `1/3*3` gives `"1"`, exact in decimal, as pratt documents;
  - `2+` gives `ok: false` with pratt's message;
  - the trace of `1+2` under `traced` holds:
    - `js_to_wasm evaluate ["1+2"]`;
    - the C++ scopes `parse` and `evaluate`, in order;
    - `return evaluate`.
- [ ] **Step 2:** Run. Expected: FAIL.
- [ ] **Step 3: Write the example.**
  - Trace scopes go around the parse and around each binary operation's evaluation, with
    their operands as arguments.
  - **The live page:** an input, the visualizer, and the options `1+2`, `1/3*3`, `2^10`.
  - **The explanation page:**
    - what crosses the bridge, and why the arithmetic is exact;
    - how to build it, the commands taken from the Jamfile with an `include::`-style copy
      checked by the site's tests, never typed twice.
- [ ] **Step 4:** Run. Expected: PASS.
- [ ] **Step 5:** Run `$B2 -a --build-dir=bin/b-site site`, serve `bin/site/`, and play
  the calculator in Chrome:
  - at 360, 768, 1280 and 1440 px, light and dark;
  - sound on and muted;
  - with reduced motion.

  Fix every visual fault before the commit.
- [ ] **Step 6: Commit** (site): `"The calculator: the first example, pratt in the
  visualizer"`.

## Task 18: The site in headless Chrome

**Files:**
- Create (site):
  - `test/browser/run.mjs`: Chrome over DevTools, as
    `libs/trystero/test/oracle/interop/run.mjs` finds and drives it;
  - `test/browser/scenarios.mjs`.
- Modify (site): `Jamfile`, the target `browser-test`.
- Modify (superproject): `tools/ci/matrix.py` (a nightly lane `site-browser`) and
  `matrix_test.py`.

- [ ] **Step 1: Write the failing scenarios:**
  - every page of `bin/site/` loads with no console error and no failed request, at
    360 px and at 1280 px (`Emulation.setDeviceMetricsOverride`, mobile on for 360);
  - the calculator page plays `1+2` to `done`, and its log pane ends with the result `3`;
  - with JavaScript disabled (`Emulation.setScriptExecutionDisabled`), the home shows the
    `<noscript>` message;
  - **the PWA:**
    - a first visit asks to install, naming the size from `precache.json`;
    - accepting caches every file of it, which Cache Storage then lists;
    - the site then loads with the network offline (`Network.emulateNetworkConditions`);
    - a second visit does not ask;
    - after one file of `bin/site/` changes and `b2 site` runs again, the next visit
      downloads that file alone;
  - **speed, which ranking measures:** on the home, the libraries page and the
    calculator, at 360 px with mobile emulation and at 1280 px, through
    `PerformanceObserver`:
    - Largest Contentful Paint under 2.5 s;
    - Cumulative Layout Shift under 0.1;
  - with the wasm request blocked (`Network.setBlockedURLs` on `*.wasm`), the visualizer
    shows the failure as text and no console error is uncaught (Review Focus 2).
- [ ] **Step 2:** Run with no site built. Expected: FAIL, naming `bin/site/`.
- [ ] **Step 3:** Implement `run.mjs`:
  - serve `bin/site/` on `127.0.0.1` with node's `http` module;
  - start `chrome-headless-shell` 155.0.8059.39 with `--no-sandbox`;
  - drive it over DevTools;
  - leave no process behind (`tools/drive/drive.py` bounds it when run through b2).
- [ ] **Step 4:** Run after `b2 site`. Expected: PASS, every scenario.
- [ ] **Step 5: Commit** both repositories: `"The site in headless Chrome: every page,
  the calculator, the PWA offline and updated, without its wasm"`.

## Task 19: Final review and handover

- [ ] **Step 1:** Run, each from scratch:
  - `$B2 -a --build-dir=bin/b-site site`;
  - `npm run build`, `npm test` and `npm run dev` in the site alone, which must work
    without the superproject's b2;
  - `$B2 -a /webcpp/site//test`;
  - the browser lane;
  - xstate's four lanes;
  - the lint;
  - every `tools/**/*_test.py`, each as `python3 <path>`, as the CI's tools job runs them.

  Record each result in the ledger.
- [ ] **Step 2:** Dispatch the whole-branch review (both repositories and the libraries
  changed), on the most capable model, with this plan's Review Focus.
- [ ] **Step 3:** Fix what it finds, each with a failing test first.
- [ ] **Step 4 (controller, G2):** with the owner's agreement, push:
  - the libraries;
  - the site;
  - the superproject.

  Then watch the CI publish the preview, open https://webcpporg.github.io/ and
  https://webcpporg.github.io/webcpp/libs/pratt/, and check that the second lands on
  https://webcpporg.github.io/libs/pratt/.
