## Context

The site's increment 1 (the repository, `b2 site`, the eight pages, the libraries' pages, the
report, publication, the redirect) is built and live; the genesis of the organization records it.
The site specification (`reference/site-spec.md`, approved 2026-10-10) names six more increments;
this change takes increments 4 and 5, planned in `reference/plan.md` (revision 3, 2026-10-11, after
an independent review and the controller's rulings), whose tasks continue the site plan's numbering
at Task 20. It builds on increments 2 and 3 (xstate on emscripten and the site's wasm, the PWA, the
trace, the visualizer, the calculator, the browser lane), of which a part exists on a local branch.

## Goals / Non-Goals

**Goals:**

- Trystero's own collaborative cursors run on the site on webcpp::trystero compiled to WebAssembly,
  behind a package whose exports and objects are Trystero 0.26.0's.
- The site's state machines, a server handler served as a WASI component and natively, and a C++
  program driving the page run as examples, each with its explanation, its gallery entry and its
  browser scenario.
- Every size the pages show is a number the build measured.

**Non-Goals:**

- File transfer and video chat (increment 6, as trystero's roadmap delivers them), Google search
  (increment 7, webcpp.org's) and the release channel (increment 8).
- Publishing the package on npm (gate G6).
- trystero's move onto webcpp::asio, webcpp::webrtc and webcpp::crypto, which changes the package's
  files (it will add the JSPI and Asyncify variants), never its surface.
- A native host in webcpp::wasi: a host comes with webcpp::beast.

## Decisions

### D1. The JavaScript package belongs to trystero; the site hosts it unbundled
The package lives in `libs/trystero/js/` and is built by a trystero target; the site copies it to
`packages/trystero/` and an example page reaches it through an import map, so that the page runs the
very bytes a JavaScript developer gets. Alternative rejected: a copy built into the site, which a
library artifact would not be.

### D2. The surface is Trystero 0.26.0's, and what is not implemented is refused
Every member of Trystero's configuration and options is implemented or refused with a `TypeError`
naming it, none accepted in silence (a relay-only ICE policy ignored would expose the address it
asks to hide). Events are copied out of the wasm and delivered on a microtask, as Trystero does, so
that a handler may leave its room or send at once. One addition, `observe`, hands copies of each
crossing to the site's visualizer.

### D3. Built on the shared connection
The package uses `app` alone (one `selfId`, one connection per peer across an app's rooms), as
Trystero does; the shared connection's integration into trystero's `main` is a precondition.

### D4. Trystero's sketch keeps its bytes but its import line
trystero.dev publishes a 24-line sketch, not a page; its code is in no repository and the page states
no licence. The copy keeps provenance (URL, date, hashes, extraction rule), relies on the owner's
ruling that everything of Trystero is MIT, keeps `LICENSE-TRYSTERO.txt` beside it, and is held to
its SHA-256 by a test. The three helpers the sketch leaves to the application are the site's, and
treat a peer's messages as hostile.

### D5. Nothing connects on load; the reader's Join does, under a notice
Trystero's examples need public Nostr relays and STUN servers, which see the reader's address as
peers do. No page connects on load; an example starts only on the reader's gesture, under the notice
of gate G7. Tests and CI reach no public relay: Chrome maps every host but 127.0.0.1 to not found, a
canary proves it, and the relay is the interop lane's.

### D6. Sizes are measured by the build, not committed
A committed size file would couple the site's repository to every measured library and to the build
machine: any byte a library's wasm moves would turn `main` red until the site regenerated it. Every
`b2 site` measures into `bin/site/sizes.json` and a check holds every number a page shows to it.
This is the controller's recommendation for gate O5 and the plan is written for it.

### D7. Frames for live surfaces
A live example's surface is a page of its own (`Frame`), not indexed on either channel, without a
card, shown in an `<iframe>` of the example page, so that the visualizer sits beside it and plays the
crossings the package reports.

### D8. On the server: one handler, two mains, one transcript
`answer(method, target)` is one function; the component's main and a native Boost.Beast host both
call it, and both answer the same `.requests` with the same `.expected`, checked in the site's own
lane `http` on native, wasip2 and wasip3. `b2 site` has neither wasi-sdk nor wasmtime, so its
terminal sessions are built from the committed transcript, never typed.

### D9. C++ calls the browser is a program on webcpp::asio, and is last
The owner's decision makes webcpp::asio the way the browser's promises and events reach C++, so no
version on `emscripten::val` alone is written; the task waits for the asio track's publication.

### D10. The site's own lanes
A Jamfile under `site/examples` may declare lanes, which the CI runs as a library's own lanes; the
site's programs never reach `declared-targets`, so no ordinary lane is planned for them.

## Risks / Trade-offs

- **Preconditions on other tracks.** The package needs the shared connection in trystero's `main`;
  Task 33 needs webcpp::asio published; tasks 26 to 29 need the site's earlier tasks 12 to 18.
  The plan records which tasks may start without each.
- **A page that runs a stranger's messages.** Review Focus 1: a JSON `null`, a colour holding CSS or
  markup, a megabyte string, hundreds of peers; answered by the helpers and the facade's reporting.
- **Browsers that cannot run the package** (the wasm blocked, an insecure context, no WebRTC): text
  in the frame, an explanation in the visualizer, no uncaught error.
- **The server's input.** An expression of 4,097, 6,000 or 100 KB bytes, bytes that are not UTF-8: the
  same 4xx with the same text from wasmtime and from the native host.
- **Public relays may be unavailable;** the only run that reaches them is manual, after G7.
- **The cursors sketch sends on every pointer move,** unthrottled, as written; the page says what
  crosses the bridge per move.

## Migration Plan

Increment 4 first (the shared connection into trystero's `main`, then trystero's tasks 22 to 25, then
the site's 26 to 28, then 29), each task a commit in trystero or the site and then a commit of the
superproject's pointer; increment 5 as each example ships alone (30 to 33), then 34 and 35. The
controller pushes only with the owner's agreement (G2), after each increment's review. Archiving
this change modifies `openspec/specs/site/spec.md`.

## Open Questions

- **G4.** Does the owner approve the specification of trystero's JavaScript API in advance, naming
  `observe`, the refusal list and the microtask delivery?
- **G6.** Publication of `@webcpp/trystero` on npm: from CI only, with provenance, under an
  organisation enforcing two-factor authentication, after one review cycle on the preview?
- **G7.** The public network from the site: (a) Trystero's app id and room (the recommendation),
  (b) a webcpp app id, or (c) no live join on the preview, only a recorded trace.
- **O5.** The sizes: measured by every build (the recommendation) or a committed file.
- **R12.** File transfer's actual needs (`onReceive`, `onReceiveProgress`, metadata, `onProgress`; no
  request action) are recorded for increment 6, to tell the owner when it is planned.
