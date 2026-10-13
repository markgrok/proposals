---
title: "webcpp site: references the owner chose (Boost, Apple, trystero.dev, effect.website, OpenUI, Claude Design)"
tags: [webcpp, site, design, research]
status: active
created: 2026-10-10
---

# webcpp site references

## effect.website (seen 2026-10-10)

- Dark, grid-lined background, mono uppercase labels ("// IN PRODUCTION AT"), a large sans
  headline ("Reliable TypeScript for the AI era"), an install box.
- The example showcase, "inspired by Kit Langton's Visual Effect":
  - Tabs by theme (Concurrency, Schedule, Error handling, Constructors, Scope), then one
    sub-tab per function (Effect.all, Effect.race, ...).
  - A run button, and options as a segmented control (sequential, numbered, unbounded).
  - Each effect is a tile: idle (a plus), running (a glowing blue tile), done (green, with
    its value, "65°"). An arrow leads to a result bar that fills as values arrive.
  - The code sits below the tiles.

## trystero.dev (seen 2026-10-10)

- Black, a serif display face ("The web is the network"), a tan accent, an illustration of
  two smiling computers shaking hands.
- Sections:
  - "Connect in a few lines": code to run in two tabs;
  - one feature per section, each with a short code block: rooms, encryption, signaling
    choices, actions, media, requests;
  - guides that compare (WebRTC vs WebSockets, Trystero vs PeerJS).

## OpenUI (github.com/thesysdev/openui, MIT, ~10.7k stars, last commit 2026-10-11)

- OpenUI Lang is a compact language for UI, first meant for model-generated UI:

  ```
  root = Stack([header, content])
  header = CardHeader("Hello")
  content = TextContent("World")
  ```

- Components:
  - defined with `defineComponent({ name, description, props: z.object(...), component })`;
  - collected with `createLibrary`;
  - a spec JSON comes from `openui generate`.
- Runtimes:
  - `@openuidev/lang-core` (parser, one-shot or streaming, `$variables`, reactive
    expressions, `mergeStatements`);
  - `@openuidev/react-lang` 0.4.1 (`<Renderer library response isStreaming />`, `onAction`,
    `onStateUpdate`, `initialState`);
  - Vue, Svelte and Angular runtimes.
- Static rendering (renderToString at build time) is not documented: to prove in a spike.

## Claude Design (Anthropic Labs, April 2026)

- Imports design systems from GitHub repositories and builds with their components.
- Exports to standalone HTML and hands off to Claude Code.
- `/design-sync` pulls a design system in from the terminal.

## Google

- Programmable Search Engine, restricted to the site, embedded with `cse.js?cx=<id>`.
  Creating the engine needs the owner's Google account.
- Search Console verifies the site with an HTML file or a meta tag.
- `sitemap.xml` and `robots.txt` tell crawlers what to index.
- The robots token `Google-Extended` governs use for Google's AI models.

## Spike: OpenUI Lang to static HTML (2026-10-10)

- Scratch project with `@openuidev/react-lang` 0.4.1, React 19.3.0, zod 4.6.5 and tsx.
- Components `Stack`, `Heading` and `Paragraph`, defined with `defineComponent`; children
  are typed as `z.array(z.union([Heading.ref, Paragraph.ref]))` and drawn with
  `renderNode`.
- The page `root = Stack([hero, lead])` was rendered with `renderToString(<Renderer
  response={page} library={library} isStreaming={false} />)`, in node, with no browser.
- The output was
  `<main><h1 class="hero">Where C++ and the web meet</h1><p>...</p></main>`, inside two
  wrapper divs of the Renderer (`aria-busy`, an opacity transition), which
  `Renderer.Root` with `Renderer.Content` can trim.
- Positional arguments fill the props in the schema's order.
- Conclusion: static rendering at build time works.

## trystero.dev's examples

- Collaborative cursors (actions), video chat (media streams), file transfer (actions,
  files, requests).
- Their code is not in github.com/dmotz/trystero; the published pages carry it. The owner
  confirmed on 2026-10-10 that everything of Trystero is MIT, these examples included.
