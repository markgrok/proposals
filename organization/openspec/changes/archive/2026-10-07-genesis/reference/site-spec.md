---
title: "The webcpp site: preview at webcpporg.github.io, release at webcpp.org, built by webcpp"
tags: [webcpp, site, docs, emscripten, spec]
status: active
created: 2026-10-10
---

# The webcpp site: specification (approved by the owner, 2026-10-10; revised the same day with his corrections: a JavaScript application, a PWA, npm through b2)

## 1. Purpose

The site is where webcpp shows its idea, and then leads its reader to the libraries'
documentation and to the examples. The idea: C++ and the web meet. The best of C++ runs in
a browser, on a server as a WebAssembly container, and natively; and the libraries web
developers use most are brought to C++.

The site speaks to four readers:

- C++ developers;
- JavaScript developers, who get more efficiency, control of memory, speed and smaller
  downloads;
- crypto developers who build decentralized applications;
- AI agents, which read it to find libraries of high quality.

The site is built with webcpp. Its examples run on webcpp's libraries, never on the
JavaScript originals, and its own logic runs on webcpp::xstate compiled to WebAssembly.
Anyone who opens its repository on GitHub sees webcpp built with webcpp.

## 2. The owner's decisions (2026-10-10, ~/.buzz/PLANS/WEBCPP_DESIGN.md)

1. The site on GitHub Pages is the preview: what `main` holds. It moves to the
   organisation's root, https://webcpporg.github.io/.
2. webcpp.org will serve the latest release, from the first release on. The domain was
   bought on 2026-10-07 at GoDaddy; its DNS moves to Cloudflare when a release site exists.
3. One command builds the whole site into the build output, in either channel.
4. Information architecture after boost.org; visual style after Apple; "where C++ and the
   web meet"; a logo that joins C++'s symbols with the web's. trystero.dev is a liked
   reference. A section for JavaScript developers always.
5. Examples are shown the way effect.website shows its own: working, with sound, and with a
   visual communication of each step. The code is highlighted as it runs, with its
   parameters and logs. The reader sees:
   - the JavaScript that calls into the wasm module;
   - the C++ that runs inside it;
   - the JavaScript functions the wasm dispatches, with the user's input.
6. Libraries are listed as Boost lists them. Search is Google's search restricted to the
   site, and the site carries the files Google needs to index it.
7. The site is static files, no server, and a JavaScript application: the browser loads
   React and OpenUI's renderer, and all of the site's content is rendered by OpenUI from
   OpenUI Lang. Little HTML is written by hand. Its components are written in OpenUI Lang
   as far as possible, so that Claude Design can rearchitect it later. The same
   infrastructure builds the example and demonstration programs, and the site hosts the wasm
   they produce. (Revised by the owner on 2026-10-10: the site is not meant to work without
   JavaScript.)
8. The site shows:
   - C++ libraries used from JavaScript, and the reverse: JavaScript functions exported to
     C++;
   - every wasm's size beside its JavaScript original's;
   - how to run the same code on a server, natively and as WASI.
9. The trystero examples are Trystero's own (trystero.dev), running on webcpp::trystero.
10. The site's state machines run on webcpp::xstate, not on XState's JavaScript.
11. The site's source and its examples live together in the site's repository.
12. The site is a PWA, mobile first, and works well on the desktop too (section 10). At
    start it checks whether all of its artifacts are installed; when not, it asks the reader
    whether to install them; it caches everything, every wasm included, so that a return
    visit loads at once.
13. The build is b2's, entirely. b2 calls npm's scripts through a wrapper to build the
    JavaScript parts. Those parts can also be built and run with npm directly, but the site
    is always deployed through b2.
14. The repository speaks to two readers (section 4):
    - a JavaScript developer, with npm alone for the examples and pages, or the whole
      project taught step by step, b2 included;
    - a C++ developer, with b2 alone, never npm.

## 3. Channels and the command

| Channel | Built from                                 | Served at                    | When                   |
| ------- | ------------------------------------------ | ---------------------------- | ---------------------- |
| preview | `main` of the superproject and of the site | https://webcpporg.github.io/ | every push to `main`   |
| release | a release tag                              | https://webcpp.org/          | from the first release |

`b2 site`, run at the superproject's root, builds the whole site into `bin/site/`.
`b2 site -sWEBCPP_SITE=release` builds the release channel. Any other value stops the
build, named.

- A preview page carries a band: "Preview of main at `<commit>`, `<date>`", with a link to
  the latest release once one exists.
- A release page carries the release's version.

Everything the site serves is built by this command, as the CI builds it. The test matrix
comes from the CI's lanes. Locally, without results, `report/` says that no results were
given.

b2 builds the JavaScript parts by calling the site's npm scripts, through one wrapper rule
(`site.npm <script> : <arguments>`), and never reimplements them. The same scripts run on
their own for work on the site:

| npm script                   | Does                                                                                                                                                                  |
| ---------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `npm run build -- <options>` | bundles the client and renders the pages; b2 passes the catalogue, the channel and the output directory                                                               |
| `npm test`                   | the site's own tests                                                                                                                                                  |
| `npm run typecheck`          | TypeScript's check                                                                                                                                                    |
| `npm run dev`                | builds the site's own pages with a fixture catalogue, serves them on 127.0.0.1 and rebuilds on a change: for working on pages and components without the superproject |
| `npm run serve -- <dir>`     | serves a built site on 127.0.0.1                                                                                                                                      |

`b2 site-serve` builds `bin/site/` and serves it on 127.0.0.1, its address printed before it
starts. Publication always takes `b2 site`'s output; nothing built by npm alone is
deployed.

## 4. Repository and publication

- **The site's repository:** `webcpporg/webcpporg.github.io`, BSL-1.0, checked out in the
  superproject as the submodule `site/`, as a library is under `libs/`. It is a project
  that uses webcpp's libraries the way any user would.
- **Publication:** the superproject's CI already builds every library's page and the
  report. On a push to `main`, it builds `b2 site` and pushes `bin/site/` to the site
  repository's `gh-pages` branch, with a deploy key.
  - The key is a secret of the superproject, and the public half is a deploy key of the
    site repository.
  - The controller creates both once the owner agrees.
- **The old address:** the superproject's own Pages, https://webcpporg.github.io/webcpp/,
  becomes one page that redirects every path to the same path at the root, so that no
  published link breaks.
- **Checks on pull requests:** the site repository's own workflow builds the site on every
  pull request, without publishing.

### Two ways in

The repository's README has two sections, one per reader, and neither sends its reader
through the other's tools.

- **For JavaScript developers.**
  - **Path 1, the examples and pages, with npm alone:** `npm ci`, `npm run dev`,
    `npm test`.
    - `npm run dev` uses the examples' wasm built locally when `bin/site/` holds it, and
      otherwise the wasm the published preview serves. That wasm is downloaded once into
      `build/dev/` and checked against the preview's `precache.json`.
    - A JavaScript developer changes pages, components and the examples' JavaScript
      without a C++ toolchain.
  - **Path 2, the whole project:** taught step by step, as for someone who has never built
    C++:
    - install b2, Boost, the emsdk and node, each with the exact version and the command;
    - clone the superproject with its submodules;
    - `b2 site-serve`.
- **For C++ developers.** b2 alone:
  - `b2 site`, `b2 site-serve` and the test targets;
  - where the site's C++ lives (`app/`, `examples/`, `include/webcpp_site/`);
  - the prerequisites, node among them as one more tool b2 runs, the way it runs Python.

  npm's commands, `package.json` and the bundler are never named in this section.

- **Kept true by a test:**
  - every npm command the README shows is a script of `package.json`;
  - every b2 command it shows is a target the build declares;
  - the C++ section names no npm command.

Layout of the site repository:

```
webcpporg.github.io/
  Jamfile                   the site's targets; b2 site at the superproject runs them
  package.json, lock        node dependencies, pinned (React, OpenUI, zod, esbuild)
  components/               the React component library, each with its zod props: the design
                            system Claude Design imports
  pages/                    the pages, in OpenUI Lang (*.oui)
  machines/                 the site's state machines, XState's JSON, run by webcpp::xstate
  app/                      the C++ of the site's own wasm: the machines, the trace bridge
  include/webcpp_site/      trace.hpp, the trace points of section 6
  examples/<name>/          one directory per example: its C++, its page, its explanation
  src/                      the client: the OpenUI renderer's entry, the service worker, the
                            install flow (section 10)
  tools/                    build.mjs (bundles, renders, writes the precache list), sizes.mjs
                            (section 8)
  test/                     the site's checks (section 11)
```

## 5. Information architecture

Each line is a page or a section of the navigation, after boost.org's.

- **Home.**
  - "Where C++ and the web meet", and the logo.
  - The same code in three places: the browser, a server as a WASI container, and native.
  - A live example above the fold, in the visualizer of section 6.
  - The three doors (C++, JavaScript, AI).
- **Libraries.** A table as Boost's. Each row gives:
  - the name, linked to the library's page;
  - the description, the category and the targets;
  - what it ports, at which version, linked to the original;
  - its wasm's size beside the original's (section 8).
  - It is generated from every library's `meta/libraries.json`.
- **For C++ developers:**
  - getting started;
  - building for each target;
  - the libraries' pages.
- **For JavaScript developers:**
  - using a webcpp library from JavaScript;
  - exporting JavaScript functions to C++;
  - memory, speed and size, measured;
  - running the same code on a server, natively and as WASI with wasmtime.
- **For AI developers:**
  - `llms.txt`, and `libraries.json`, the catalogue for agents;
  - webcpp's integrations with the AIs, as they come.
- **Examples.** A gallery. Each example has a live page in the visualizer and a page that
  explains it.
- **Documentation:** each library's page and API reference, at `libs/<name>/`, as built
  today, in the site's theme.
- **Report:** the test matrix.
- **Community:** CONTRIBUTING.md, the rules, how to bring a library.
- **Search:** Google's Programmable Search, restricted to the site (section 9).

## 6. The bridge visualizer

The signature of the site. An example's page runs the example's real wasm and shows, in
step, four panes:

| Pane       | Shows                                                                                                                                                                                                                     |
| ---------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| JavaScript | the example's JavaScript, with the line that runs highlighted                                                                                                                                                             |
| Bridge     | each crossing as a tile, after effect.website's: a call from JavaScript into a wasm export, or from the wasm into a JavaScript import, with its name, arguments and result; idle, running (lit) and done (with its value) |
| C++        | the C++ inside the wasm, with the scope that runs highlighted and its parameters                                                                                                                                          |
| Log        | what the program logs, and each crossing, in order                                                                                                                                                                        |

### The trace

- **The JavaScript side:** the loader wraps the module's exports and imports. Every call
  across becomes an event: `js_to_wasm`, `wasm_to_js`, `return`, with name, arguments,
  result and a monotonic sequence number.
- **The C++ side:** an example marks the scopes worth showing with
  `WEBCPP_SITE_TRACE_SCOPE("name", args...)` from `include/webcpp_site/trace.hpp`.
  - Inside the site's build, the macro sends `cpp_enter` and `cpp_exit` events, with the
    file and line, through one JavaScript import.
  - Elsewhere it compiles to nothing.
  - Trace points are chosen by hand: deterministic, readable, and what the reader should
    see. Reading DWARF out of the wasm was considered and set aside, as far more work for
    a worse view.
- **Ordering:** the events are recorded live and played back at a pace a person can follow.
  The program itself runs at full speed; the view replays its trace.

### The player

- Play, pause, step, speed and restart, with the example's options as a segmented control,
  as effect.website's.
- Its state machine is `machines/player.json`, run by webcpp::xstate in the site's wasm:
  the site visualizes itself, and one page shows the player's own machine in the
  visualizer.
- **Sound:**
  - short tones synthesized with Web Audio, one per kind of event, with no audio files;
  - muted until the reader starts a run, which is the gesture browsers require;
  - a mute control remembered per reader.
- **Accessibility:**
  - with `prefers-reduced-motion`, tiles change state without animation;
  - every event is also text in the log pane;
  - the player works from the keyboard.

## 7. Examples

Each is a directory of the site's repository, built for emscripten by `b2 site`, with its
live page and its explanation page.

| Example               | Shows                                                                                                               | Needs                                                                     |
| --------------------- | ------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------- |
| Calculator            | JavaScript calls C++ with the user's input; pratt parses and evaluates, exact in decimal; the first visualizer page | pratt (emscripten, today)                                                 |
| State machine         | the same XState JSON run by webcpp::xstate in wasm, beside the browser's own state                                  | xstate for emscripten (section 12, increment 2)                           |
| C++ calls the browser | JavaScript functions exported to C++: C++ drives the page                                                           | the trace and the loader                                                  |
| Collaborative cursors | Trystero's own example, its HTML and JavaScript unchanged but for the import, on webcpp::trystero                   | trystero's JavaScript API (below)                                         |
| File transfer         | Trystero's own example                                                                                              | trystero's request actions, transfer decisions and progress (its roadmap) |
| Video chat            | Trystero's own example                                                                                              | trystero's media (its roadmap)                                            |
| On the server         | the same program as a WASI component, served by wasmtime, and natively; shown as recorded terminal sessions         | wasi                                                                      |

- **trystero's JavaScript API.** The trystero examples import a module with Trystero's own
  JavaScript API (`joinRoom`, `makeAction`, `onPeerJoin`, ...), backed by webcpp::trystero
  compiled to wasm. The page changes one line: the import.
  - The module belongs to trystero, as its face for JavaScript developers. Its
    specification is trystero's (an increment of that library, not of the site).
  - The three examples of trystero.dev become trystero's measure of a complete port.
  - Their code is Trystero's, MIT like all of Trystero (the owner, 2026-10-10). Each ported
    example keeps Trystero's notice in `LICENSE-TRYSTERO.txt` beside it, as a port does.

## 8. Sizes, measured

- For every library that ports a JavaScript original, the build measures two sizes: the
  original bundled and minified with esbuild, and the library's wasm with its glue. Each is
  measured raw, gzip and brotli.
- They appear on the library list and on the library's page. A number is never written by
  hand.
- Speed comparisons come later, from the libraries' benchmarks, measured the same way.

## 9. Search and indexing

The preview (webcpporg.github.io) is the development version and is not indexed (owner,
2026-10-11): every page of the preview channel carries `<meta name="robots" content="noindex">`,
and the preview's robots.txt names no sitemap and writes none. What follows is the release
channel's, webcpp.org.

- `robots.txt` allows every crawler, `Google-Extended` included, so that Google's AI learns
  webcpp's documentation, and names the sitemap.
- `sitemap.xml` lists every page, generated by the build.
- Search Console: the owner adds the property `https://webcpp.org/` with the release site
  (increment 8; owner, 2026-10-11: the preview gets none), and the release site
  carries the verification file Google gives.
- The search box is Google's Programmable Search Element, restricted to the site, with the
  engine id (`cx`, not a secret) the owner creates.
  - Until it exists, the box links to a Google search with `site:webcpp.org`.
- `llms.txt`: what webcpp is, and a link and one line per library and per page.
- `libraries.json`: the catalogue, from the libraries' meta.

### SEO

The aim (the owner, 2026-10-10): Google ranks webcpp for C++ developers who search for what
exists to develop C++ for the web.

- **The queries the site answers** are data, `seo/queries.json`: each query with the page
  that answers it. The first set:
  - "C++ for the web", "C++ in the browser", "C++ WebAssembly libraries";
  - "Emscripten C++ library", "WASI C++", "C++ wasmtime", "WebAssembly containers C++";
  - "header-only C++20 libraries";
  - "C++ WebRTC", "C++ peer to peer browser";
  - "C++ state machine XState";
  - "C++ Pratt parser";
  - "use C++ from JavaScript", "call JavaScript from C++".

  Each library adds the queries of its field. A test checks that each query's words appear
  in its page's title, `<h1>` or description.
- **Every page's head**, written by the shell from the page's `Page(title, description,
  ...)` and the catalogue:
  - `<title>`: at most 60 characters, the page's subject first, then "webcpp";
  - `<meta name="description">`: 120 to 160 characters;
  - `<link rel="canonical">`, absolute, by channel;
  - `<meta name="robots" content="index, follow">` and `<html lang="en">`;
  - Open Graph: `og:title`, `og:description`, `og:url`, `og:type`, `og:site_name`,
    `og:image` with its width, height and alt;
  - `twitter:card` `summary_large_image`, with its title, description and image;
  - `theme-color`, the manifest and the icons;
  - JSON-LD, one `@graph` per page:
    - `WebSite`, with a `SearchAction` to the search page;
    - `Organization`, webcpp: logo, `sameAs` GitHub;
    - `WebPage` and `BreadcrumbList`, from the page's path;
    - on the libraries page, an `ItemList` of `SoftwareSourceCode`;
    - on an example, `SoftwareSourceCode` and `HowTo` for its explanation page.
- **The libraries' pages** (Asciidoctor) get the same head from `assemble.py`, from the
  catalogue:
  - their title, their description;
  - `SoftwareSourceCode`:
    - `programmingLanguage` "C++";
    - `codeRepository`;
    - `license` the BSL;
    - `runtimePlatform` from their targets (WebAssembly, Emscripten, WASI, native);
    - `isBasedOn` the original of a port;
  - `TechArticle`.
- **Social cards:** one image per page, 1200 by 630, rendered by the build from an SVG
  template with the page's title and the logo, with `@resvg/resvg-js`.
- **The body:**
  - one `<h1>` per page;
  - headings in order;
  - every image with `alt`;
  - links with text that says where they go;
  - the prerendered text first in the document.
- **Speed, which ranking measures**, is checked in the headless Chrome lane on the local
  server:
  - Largest Contentful Paint under 2.5 s;
  - Cumulative Layout Shift under 0.1, by `PerformanceObserver`;
  - every image and the visualizer reserve their size.
- `<meta name="keywords">` is not written: Google ignores it.

## 10. Technology

| Part          | Choice                                                                                                                                                                                                                                                                                                                             | Why                                                                                   |
| ------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------- |
| Pages         | OpenUI Lang, rendered in the browser by OpenUI's React renderer; each page's document is a small shell that carries its OpenUI Lang source and the client bundle                                                                                                                                                                   | the owner's decision 7                                                                |
| First paint   | the build also renders each page's OpenUI tree with the same renderer and components (`renderToString`, proven by the spike of 2026-10-10) into the shell, and the client hydrates it: the page shows before the scripts run, and a crawler that runs no script still reads the text. It is OpenUI's output, never written by hand | speed, the owner's "loads at once", and indexing                                      |
| Components    | React 19 with zod props (`defineComponent`, `createLibrary`)                                                                                                                                                                                                                                                                       | OpenUI's model; the design system Claude Design imports from the repository           |
| Logic         | webcpp::xstate in the site's wasm                                                                                                                                                                                                                                                                                                  | the owner's decision 10                                                               |
| Bundling      | esbuild, pinned                                                                                                                                                                                                                                                                                                                    | one binary, no configuration to speak of                                              |
| Library pages | Asciidoctor, as today, in the site's theme; their header is the site's, rendered by OpenUI in the page                                                                                                                                                                                                                             | the libraries' documentation system stays one; the site's frame around it is OpenUI's |
| PWA           | a web app manifest; a service worker of the site's own; the install flow as a machine on webcpp::xstate                                                                                                                                                                                                                            | the owner's decision 12                                                               |
| Icons         | rendered from the logo's SVG by the build, PNG at 192 and 512 px and an Apple touch icon, with `@resvg/resvg-js`, pinned                                                                                                                                                                                                           | iOS needs PNG; no image is drawn twice                                                |
| Build         | b2 drives Emscripten, and npm's scripts through one wrapper; node and its packages pinned by the lockfile                                                                                                                                                                                                                          | the owner's decision 13                                                               |

No server, no runtime API, no cookie. The only third-party scripts are Google's search
element, on the search page, and nothing else. A browser without JavaScript gets one
message, in `<noscript>`, saying that the site needs it.

### Mobile first, and the desktop

- **Layouts are written from the narrowest screen up:** 360 px first, then 768, 1280 and
  1440.
- **Every page is checked at each width.** On a phone:
  - the visualizer shows one pane at a time, with tabs between JavaScript, the bridge, C++
    and the log;
  - the navigation folds into a menu.
- **On the desktop,** where developers read most, the panes stand side by side and the code
  has room.

### The PWA

- **The manifest:** `manifest.webmanifest`:
  - `name` "webcpp", `start_url` and `scope` "./";
  - `display` "standalone";
  - the theme colours from the design's tokens;
  - the icons of the table above.
- **The precache list:** `precache.json`, written by the build. It lists every file of
  `bin/site/` with its SHA-256 and size, and the total. Its version is the SHA-256 of the
  list.
- **The service worker:** `sw.js`, at the root, the site's own, about a hundred lines and
  tested.
  - Precached files are served from the cache first.
  - A navigation falls back to the cached page when the network fails.
  - Any other same-origin GET goes to the network, and to the cache when the network
    fails.
  - Once the site is installed, it works offline.
- **The install flow** is a machine, `machines/install.json`, run on webcpp::xstate in the
  page:
  - **checking:** compare the cache with `precache.json`;
  - **ready:** when every file is there;
  - **asking:** when files are missing. The reader is asked whether to install the site for
    instant and offline use, with the size to download (from `precache.json`, never typed).
  - **installing:** shows progress in bytes; then `navigator.storage.persist()` is
    requested, so the browser keeps the cache;
  - **declined:** remembered for this version, in `localStorage` inside `try`/`catch`.
    The site still works from the network.
  - **updating:** a new `precache.json` downloads only the files whose hash changed, in the
    background, then offers a reload.
- **Installing to the home screen:**
  - where the browser offers it (Chrome, Edge and Android, through `beforeinstallprompt`),
    the ready state offers "Install the app";
  - on iOS Safari, which has no prompt, it shows how to add the site to the home screen.

## 11. Testing

- **Every page:**
  - renders at build time, its OpenUI Lang parsed and validated, an unknown component
    failing with its file and line;
  - every link resolves (a link check over `bin/site/`);
  - every page is valid HTML.
- **The PWA:**
  - the manifest is valid;
  - `precache.json` lists every file of `bin/site/` and no other;
  - the service worker's strategies are tested in node against a fake cache;
  - the install flow's machine is tested in C++, and against XState as its oracle.
- **In the headless Chrome lane:**
  - every page loads without a console error, at 360 px and at 1280 px;
  - a first visit asks to install; accepting caches every file of `precache.json`;
  - the site then loads offline;
  - a second visit does not ask;
  - a changed file updates alone;
  - each example runs its scripted scenario;
  - the visualizer records the expected trace.
- The trystero examples run between two pages over the local relay the interop lane
  already uses; CI reaches no public relay.
- The sizes of section 8 are regenerated and compared in a check, as other generated files
  are.
- Both channels build in CI.

## 12. Increments

1. **The site's repository and `b2 site`:**
   - the landing, the library list, the three doors (C++ filled; JavaScript and AI with
     what exists and the roadmap);
   - the libraries' pages and the report under the site's theme;
   - `robots.txt`, `sitemap.xml`, `llms.txt`, `libraries.json`;
   - mobile first, at every width;
   - publication at the root, and the redirect from `/webcpp/`.
2. **xstate for emscripten** (its machine layer), and the site's wasm running
   `machines/player.json` and `machines/install.json`: the PWA.
3. **The visualizer and the calculator example.**
4. **trystero's JavaScript API and the collaborative cursors.**
5. **The state machine, C++ calls the browser, and on the server.**
6. **File transfer and video chat,** as trystero's roadmap delivers them.
7. **Google search, for webcpp.org** (owner, 2026-10-11): the engine and the Search Console
   property are webcpp.org's, made with the release site; webcpporg.github.io is the
   development version only and gets neither.
8. **The release channel at webcpp.org,** at the first release, with the DNS move.

## 13. Visual direction

- Apple's restraint:
  - large type, much space, one accent;
  - motion only where it explains;
  - light and dark themes.
- The headline: "Where C++ and the web meet".
- The logo joins C++'s `++` with the web's sign. Its concepts are shown to the owner as
  mockups before increment 1 ends. The owner asked to be surprised.

## 14. Open

- **The owner's actions:**
  - agreeing to the deploy key (section 4);
  - the Programmable Search engine and the Search Console property (section 9).
