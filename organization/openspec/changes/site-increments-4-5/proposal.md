## Why

The site's increment 1 shows webcpp's libraries; its specification promises more: live examples
that run webcpp's own C++ in the browser, with the JavaScript that calls it and the C++ inside it
shown step by step. Increments 4 and 5 deliver the examples that prove the two directions of
webcpp's mission to JavaScript developers and to server developers: Trystero's own collaborative
cursors running on webcpp::trystero behind a JavaScript package whose surface is Trystero's, so that
a page changes only its import (increment 4), and then the site's own state machines on
webcpp::xstate, one program answering HTTP as a WASI component under wasmtime and natively, and a C++
program that drives the page through JavaScript functions the page exports to it (increment 5), with
every size measured by the build beside the JavaScript original's.

## What Changes

- **New `@webcpp/trystero` package** (trystero's JavaScript face, `libs/trystero/js/`, private at
  0.1.0 until the owner publishes it): an embind module over a new `js::runtime` and a
  `raw_payload` action type, and a facade whose exports are Trystero 0.26.0's, delivering events on
  a microtask, refusing every member it does not implement with a `TypeError`, with one addition,
  `observe`; proven against Trystero JS in trystero's interop lane.
- **The site serves the package** at `packages/trystero/`, with its C++ source, and keeps its
  JavaScript path working with npm alone (tests that need the package skip, with the reason).
- **Sizes, measured by every `b2 site`** into `bin/site/sizes.json` (never committed), shown on the
  libraries page, trystero's page and the JavaScript door, and checked against the pages.
- **Four examples** with a live page and an explanation each: the collaborative cursors (Trystero's
  sketch byte for byte but its import), the state machine, on the server, and C++ calls the browser
  (last, after webcpp::asio); frames for live surfaces; a visualizer that plays a surface's trace.
- **Superproject tooling:** `webcpp.js-test`, the site's own lanes under `site/examples`,
  `webcpp.serve-native` with `serve.py --native`, and the attribute `{webcpp-site}`.
- **BREAKING (for the site's own statements):** the site connects, on the reader's gesture, to public
  Nostr relays and STUN servers; the examples page stops being a roadmap; there are more than eight
  pages; frames are `noindex` on both channels and absent from the sitemap, `llms.txt` and the cards.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `site`: requirements added for the package, the sizes, the frames, the four examples, the
  tooling and the gates; thirteen requirements of the genesis modified (what `b2 site` does, the
  layout of `bin/site`, the JavaScript door, the examples page, the pages of the site, the
  components, the libraries page, the libraries' pages in the site, the head of every page, the
  preview's indexing, the indexing files, the social cards, and what the site does not do yet).

## Impact

- **Kind:** the organization (the site and its tooling); the package itself belongs to trystero.
- **Implementation:** not started. The plan's preconditions are partly met on a local, unpublished
  branch (`reference/STATUS.md`).
- **Dependencies:** the earlier increments (2 and 3: xstate on emscripten, the site's wasm, the PWA,
  the trace, the visualizer, the calculator, the browser lane); trystero's shared connection
  (`app`); webcpp::asio (Task 33 only); `/webcpp//boost_url` (in the superproject's `main`); wasi,
  pratt and Boost.Beast; esbuild 0.28.2 and `trystero` 0.26.0 (MIT) as the site's dev dependencies.
- **Status:** accepted; the owner's gates G4, G6, G7 and O5 are open (`design.md`).
- **Other proposals:** modifies the genesis of the organization (`site`); the trystero package will
  be specified again in trystero's own proposals; `libraries/xstate` and `libraries/pratt`
  are consumed unchanged.
