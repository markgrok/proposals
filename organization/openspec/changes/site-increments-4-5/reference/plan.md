---
title: "The webcpp site, increments 4 and 5: Implementation Plan"
tags: [webcpp, site, trystero, xstate, asio, wasi, emscripten, plan]
status: draft
created: 2026-10-11
---

# The webcpp site, increments 4 and 5: Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

Revision 3, after the independent review
(`~/dev/webcpp/.superpowers/sdd/plan-reviews/site-plan-45-review.md`, C1, I1 to I13, M1 to
M20), the controller's rulings on it (2026-10-11), and its re-review (N1 to N3, m1 to m10).

**Goal:** Trystero's own collaborative cursors run on the site, on webcpp::trystero compiled to
WebAssembly behind a JavaScript package whose surface is Trystero's (increment 4); then three
more examples run in the visualizer: the site's own state machines on webcpp::xstate, one
program answering HTTP as a WASI component under wasmtime and natively, shown as the sessions
its tests recorded, and a C++ program on webcpp::asio that drives the page through JavaScript
functions the page exports to it (increment 5).

**Architecture:**
- **trystero's JavaScript package, `@webcpp/trystero`,** belongs to trystero
  (`libs/trystero/js/`, a directory for a library's JavaScript face, accepted by the
  controller as following the owner's decision on the JavaScript API, O4):
  - a C++ runtime that joins rooms through webcpp::trystero's `app`, one app per app id;
  - an embind module over it;
  - a JavaScript facade, `index.mjs`, whose exports and objects are Trystero 0.26.0's
    (`joinRoom`, `selfId`, `makeAction`, `onPeerJoin`, ...), delivering events on a microtask
    as Trystero does.
  - It is proven against Trystero JS 0.26.0 in trystero's interop lane, from the release
    bytes it ships.
  - It stays `"private": true` at `0.1.0` until the owner publishes it (G6). Until then b2
    builds it and the site serves it.
- **The site hosts the package, unbundled,** at `packages/trystero/`, and an example page
  reaches it through an import map:
  - Trystero's sketch keeps its code but for the import line;
  - the helpers the sketch leaves to the application are the site's, in a file of their own;
  - the example runs in a frame of its own, beside the visualizer, which plays the crossings
    the package reports to it.
- **Increment 5 adds three examples:**
  - **state machine:** the site's machine host, compiled a second time with its trace scopes;
  - **on the server:** one handler served by wasmtime as a wasip2 and wasip3 component and
    natively by a Boost.Beast host. The served tests check all three against one transcript,
    from which the page draws its terminal sessions;
  - **C++ calls the browser,** last: a program on webcpp::asio (JSPI and Asyncify variants,
    loaded by asio's loader), awaiting the page's promises and events.
- **Sizes, measured,** start with trystero, then xstate: each port's files beside its original
  bundled by esbuild, raw, gzip and brotli. Every `b2 site` measures them into `bin/site/`,
  and a check holds every number the pages show to that measurement (O5).

**Tech Stack:**
- C++20, b2 5.5.3, Boost 1.92 (Asio, Beast, JSON, URL), Emscripten 6.0.11, wasi-sdk 34,
  wasmtime 47, node 26.7.0, Python 3.14.
- chrome-headless-shell 155.0.8059.39 over DevTools, as `libs/trystero/test/oracle/interop/run.mjs`
  drives it.
- The site's node packages, pinned by its `package-lock.json`, as WEBCPP_SITE_PLAN.md gives
  them, plus one dev dependency, the original being measured: `trystero` 0.26.0 (MIT), at the
  same version and integrity as `libs/trystero/test/oracle/package-lock.json`.

**Spec:** `~/.buzz/PLANS/WEBCPP_SITE_SPEC.md` (approved 2026-10-10), section 12,
increments 4 and 5; sections 6, 7, 8, 10 and 11 bind them.
- **The owner's decisions** cited below are in `~/.buzz/PLANS/WEBCPP_DESIGN.md`,
  by date and title:
  - 2026-10-08, "trystero and allocators";
  - 2026-10-10, "allocators, scope" and "when the new rules apply";
  - 2026-10-10, "the site proves webcpp on itself";
  - 2026-10-10, "the site's repository shows webcpp used by webcpp";
  - 2026-10-10, "Trystero's licence";
  - 2026-10-10, "the site is a JavaScript application, a PWA, built through b2";
  - 2026-10-10, "two ways into the site's repository";
  - 2026-10-10, "webcpp's mission";
  - 2026-10-10, "webcpp::asio" and "webcpp::beast";
  - 2026-10-11, "trystero shares one connection between the rooms of an app, as Trystero does";
  - 2026-10-11, "three documents approved in advance".
- **The research** is `~/.buzz/RESEARCH/WEBCPP_SITE_REFERENCES.md`. This plan
  adds what trystero.dev's example pages hold, seen 2026-10-11 (section "Facts checked"), whose
  fetched HTML is archived in `~/.buzz/RESEARCH/TRYSTERO_DEV_EXAMPLES/cursors.html`.

**Builds on:** `~/.buzz/PLANS/WEBCPP_SITE_PLAN.md` (increments 1 to 3). This
plan's tasks continue its numbering at Task 20. Its interfaces are used as it names them:
- the site's wasm and `machine_host` (Task 13): `loadSiteApp`, `useMachine`,
  `machines/player.json`, `machines/install.json`, the program `app//site-app`;
- the trace (Task 15): `TraceEvent`, `traced`, `WEBCPP_SITE_TRACE_SCOPE`
  (`include/webcpp_site/trace.hpp`), `createSound`;
- the visualizer (Task 16): `Visualizer(example, js_source, cpp_source, options)` and
  `frame(trace, n)`;
- the example page's shape (Task 17): `examples/<name>/` with its C++, script and `Jamfile`;
  `pages/examples/<name>.oui` and `pages/examples/<name>/explained.oui`;
- the PWA (Task 14): `precache.json`, `sw.js`;
- the browser lane (Task 18): `test/browser/run.mjs`, `scenarios.mjs`,
  `/webcpp/site//browser-test`.

**Not in this plan:**
- File transfer and video chat (increment 6, as trystero's roadmap delivers them);
- Google search (increment 7);
- the release channel (increment 8);
- publishing the package on npm, which waits for G6;
- trystero's move onto webcpp::asio, webcpp::webrtc and webcpp::crypto (the asio
  specification's increment 5), which changes the package's files (it will add the JSPI and
  Asyncify variants), never its surface;
- a native host in webcpp::wasi: the controller's ruling on Q1 is no, a host comes with
  webcpp::beast.

---

## Facts checked, 2026-10-11

Each fact this plan leans on, read from the trees and the pages it names. Commits move: Task
20 records the ones it starts from.

1. **The site's state.**
   - The site is `~/dev/webcpp-site/site`, branch `main` (at `f30b16f` when the review read
     it), the superproject's submodule `site/`. The superproject is the worktree
     `~/dev/webcpp-site` of `~/dev/webcpp`, branch `site` at `f7c96d3`, pushed to
     `webcpporg/webcpp` main.
   - Its ledger, `~/dev/webcpp/.superpowers/sdd/WEBCPP_SITE_PLAN/progress.md`, records Tasks 0
     to 10 complete. The site has no `app/`, `examples/`, `include/` or `machines/` until
     Tasks 12 to 17 make them.
2. **trystero** (`8e8911f` when this plan was written, `6c34e55` when the review read it).
   - `room` is joined standalone today (`include/webcpp/trystero/room.hpp`): one `self_id` per
     room, its `action<T>` with `on_message`, `async_send(T, send_options, token)` and
     `async_receive`.
   - Its message types include `payload`, `std::variant<std::string, boost::json::value,
     bytes>`, which decodes JSON in C++.
   - `doc/differences.adoc` and `doc/roadmap.adoc` list, as not ported: media; request
     actions; `onReceive` and progress; `onPeerHandshake`; passive rooms; the connection shared
     by the rooms of an app; the other strategies; the relay controls.
   - The browser backend runs on `browser::event_loop`, which polls from the page's events
     (`doc/browser.adoc`). The chat example (`example/chat/`) is a page whose room is C++,
     through embind.
   - Its interop lane (`test/oracle/Jamfile`, `interop/run.mjs`) runs Trystero JS 0.26.0 against
     the C++ peers in headless Chrome, over `interop/relay.mjs` on 127.0.0.1.
   - `test/allocation_test.cpp` counts the room's resources, the global `operator new` and the
     C library's allocation functions.
3. **The shared connection.** `WEBCPP_TRYSTERO_SHARED_SPEC.md` (approved) and its plan,
   `WEBCPP_TRYSTERO_SHARED_PLAN.md`, worked in `~/dev/webcpp-trystero-shared` on trystero's
   branch `shared-connection`. It gives:
   - `app(executor, factory, app_config)`;
   - `app::join(room_id, room_options) -> boost::system::result<room>`;
   - one `self_id` per app (`app_config::self_id`, 20 characters when empty);
   - the standalone `room` constructor removed (its Task 5).

   Its Task 16 pushes and merges `libs/trystero` and bumps the superproject's pointer, with all
   five increments: nothing of it reaches trystero's `main` before then.
4. **Trystero 0.26.0's JavaScript surface** (`@trystero-p2p/core/dist/types.d.mts`, pinned
   under `libs/trystero/test/oracle`):
   - `joinRoom(config, roomId, callbacks?) -> Room`; `selfId`, drawn once per realm,
     `genId(20)` over 62 characters;
   - `Room.makeAction(namespace, config?)`, which returns `{send, onMessage,
     onReceiveProgress, onReceive}`; a second call with the same namespace returns the same
     action (`actions.mjs:105-106`);
   - `ping`, `leave`, `getPeers`, `isPassive`, `onPeerJoin`, `onPeerLeave` and the media
     members;
   - the configuration types `BaseRoomConfig`, `RelayConfig`, `JoinRoomCallbacks`,
     `MessageActionConfig`, `SendOptions`, and `rtcConfig: RTCConfiguration`.

   It sends three payload formats: text 0, JSON 1, binary 2 (`action-wire.mjs:131`), and
   refuses `undefined` data (`action-wire.mjs:128`). It calls `onMessage` on a microtask and
   reports a throwing handler with `console.error('Trystero action handler error:', err)`
   (`actions.mjs:134-138`). A send warns once per target that is not joined and sends to the
   others (`room.mjs:28-35`). Rooms leave on `beforeunload` (`room.mjs:6-16`).
5. **trystero.dev's examples** are sketches, not pages: neither the npm package nor the
   repository holds them.
   - At the tag `0.26.0`, `docs/index.html` only redirects to https://trystero.dev.
   - https://trystero.dev/examples/collaborative-cursors/, fetched on 2026-10-11 (HTML
     SHA-256 `3d554609e6aedda46d691fdc8475f1bdcbcb86858af10312745e15ed41ecbad6`), holds one
     `<pre>`. Its text, by the extraction rule of Task 28, is 24 lines, SHA-256
     `da3d8f82a3b587106406fc783bbf3a2e334c713ec8defb10ef0d122d99480133`:
     - it imports `{joinRoom, selfId} from 'trystero'`;
     - it joins `{appId: 'dev.trystero.examples.cursors'}` in the room `'canvas'`;
     - it makes the action `'cursor'`, sends `{x, y, color}` as fractions of the viewport on
       every `window.onpointermove`, and sets `cursor.onMessage` (which destructures its data,
       so a JSON `null` throws) and `room.onPeerLeave`.
   - It calls three functions the page says are "yours to implement": `renderPeerCursor`,
     `removePeerCursor` and `colorForPeer`.
   - The page states no licence. trystero.dev's home says Trystero is under the MIT licence.
     The owner ruled on 2026-10-10 ("Trystero's licence") that everything of Trystero is MIT,
     the examples of trystero.dev included (O3).
   - The fetched HTML is in the disposable scratch
     `~/.claude/jobs/fcf3a902/tmp/site-plan-45/cursors.html`, now archived as
     `~/.buzz/RESEARCH/TRYSTERO_DEV_EXAMPLES/cursors.html` (the same hash).
   - File transfer uses `onReceive`, `onReceiveProgress`, metadata and `onProgress`, no request
     action. Video chat uses `onPeerStream` and `addStream`.
6. **webcpp::asio** (`WEBCPP_ASIO_SPEC.md`, `WEBCPP_ASIO_PLAN.md`, approved, worked in
   `~/dev/webcpp-asio`) gives:
   - `/webcpp//node-test`, the C++ test support under node (its Task 2), which `webcpp.run`,
     `run-fail` and `example` link, with a 120 s watchdog; `webcpp.variants` does not link it;
   - the bridge (Task 7); the loader `load(manifestUrl, options) -> {variant, module}`
     (Task 12); `async_await` (Task 17); `async_wait_event` (Task 18).
7. **webcpp::wasi** (`libs/wasi` at `850e09d`) serves a component with
   `WEBCPP_WASI_HTTP_MAIN_BEGIN(method, target)`.
   - `webcpp.serve` takes its test's name from the source's stem and checks the component's
     answers to `<stem>.requests` against `<stem>.expected`, a transcript of
     `curl -i -X <METHOD> ...` commands and answers, with `date` and `transfer-encoding` left
     out (`tools/component/serve.py`, `HOST_HEADERS`).
   - `response.hpp` compiles natively; nothing serves HTTP natively.
8. **Own lanes** are a library's only:
   - `webcpp.lane` refuses a Jamfile outside `^libs/([^/]+)/(test|example)(/.*)?$`
     (`tools/webcpp.jam`);
   - Jamroot's `library-directories` lists `libs/*/test` and `libs/*/example`;
   - `tools/lint/compile_commands.py`'s `LANE` reads `(libs/\S+)`;
   - `tools/ci/matrix.py`'s `parsed_own_lanes` accepts `libs/<library>/(test|example)`;
   - `.github/workflows/library.yml`'s `own-lanes` job runs them, after a checkout with
     `submodules: recursive`.

   The site's only lane is `site-test`.
9. **pratt** has targets native, emscripten, wasip2 and wasip3, and the decimal calculator
   (`include/webcpp/pratt/calculator.hpp`).
10. **`/webcpp//boost_url`** is not in the superproject's main; the infra track brings it.
    Boost.URL offers `<boost/url/src.hpp>` as Boost.JSON offers its own.
11. **npm:** no package `@webcpp/trystero` exists (404, 2026-10-11). A 404 does not show that
    the scope `@webcpp` is free; the owner creates the organisation `webcpp` (G5).

## Preconditions on other tracks

| Precondition | Track | Needed by |
| --- | --- | --- |
| Site Tasks 12 to 18 complete in the site's `main` | WEBCPP_SITE_PLAN | Tasks 26 to 29; Task 30 needs 12, 13, 15 and 16; Task 32 needs 17 |
| Site Task 19 (the review of increments 1 to 3 and their publication) | WEBCPP_SITE_PLAN | Task 29's publication (G2) |
| The shared connection's Task 16 integrated into webcpp `main`: trystero's `main` holds all five increments and the superproject's `main` points to it; then branch `site` rebased onto `main` (Task 20 Step 4) | WEBCPP_TRYSTERO_SHARED_PLAN | Tasks 22 to 26 and 28 |
| webcpp::asio's Task 2 (`/webcpp//node-test`) and Task 3 (names, `webcpp.variants`) in the superproject's `main` | WEBCPP_ASIO_PLAN | Task 23 Steps 1 and 2, and Task 31: all three edit `tools/webcpp.jam`, `Jamroot` and AGENTS.md chapter 9 |
| webcpp::asio's Tasks 1 to 8, 12, 13, 17 and 18, and its Task 23's publication: `libs/asio` a submodule of the superproject's `main` | WEBCPP_ASIO_PLAN | Task 33 |
| `/webcpp//boost_url` in the superproject's `main` (the infra track), or not: Task 20 records which | WEBCPP_SHARED_INFRA_PLAN | Task 32 picks its branch of R9 from that record |
| G4 resolved (the JavaScript API's specification approved) | the owner, through the controller | Task 22 on |
| G7 resolved | the owner, through the controller | Task 28 Step 6 (the only run that reaches public relays) and Task 29 Step 8 (the push that publishes the cursors page and its Join) |
| O5 resolved | the owner, through the controller | Task 27's `--check` and Steps 4 to 6; Steps 1 to 3 but `--check` may start before it |

Startable now: Task 20; Task 21 (its review and G4 take time); Task 27's size tool
(`tools/sizes.mjs` and its test, Steps 1 to 3, which need only the site's Task 1), except
`--check` and its test case, which wait for O5: the measurement is the same under either answer
to O5, and only the check differs (the pages held to the build's measurement under R16, or a
committed file compared under the alternative); Task 23 Steps 1 and 2 and Task 31 once asio's
Tasks 2 and 3 are in `main`.

Order of work:
- **Increment 4:** Task 20, Task 21, then Tasks 22 to 25 in trystero, then Tasks 26 to 28 in
  the site and the superproject, then Task 29.
- **Increment 5:** each example ships alone, with its page, its gallery entry and its browser
  scenario in its own task:
  - Task 30 (state machine) needs only the site's tasks for its example; its last step,
    xstate's size, needs Task 27's tool (I3).
  - Tasks 31 and 32 (on the server) need wasi, pratt and site Task 17.
  - Task 33 (C++ calls the browser) is scheduled last, after the asio track's publication.
  - Tasks 34 and 35 close the increment.

## Conflicts with the spec

Each is a place where the spec, the trees or the libraries disagree, with the ruling this plan
follows. The controller confirmed them on 2026-10-11, with the review's corrections; one that
changes an owner's decision goes to the owner first.

| # | The spec says | What is true | Ruling |
| --- | --- | --- | --- |
| R1 | Collaborative cursors: "Trystero's own example, its HTML and JavaScript unchanged but for the import" (section 7). | trystero.dev publishes a sketch of 24 lines, with no HTML and three helpers left to the application (fact 5). The code is in no repository: only the page carries it, and the page states no licence. | `examples/cursors/cursors.js` holds the sketch's text byte for byte but the import, which names the package. Provenance is kept so that a wrong copy is detectable: the fetched HTML archived with its URL, date and SHA-256, the extraction rule, and the text's SHA-256 (Task 28). The README records that the page states no licence and that the copy relies on the owner's ruling of 2026-10-10, "Trystero's licence" (O3). The three helpers are the site's (`draw.js`, BSL-1.0), defined before the sketch runs, so the sketch needs no second change. The HTML is the site's frame page. Trystero's notice stays beside it, in `LICENSE-TRYSTERO.txt`. |
| R2 | "The trystero examples import a module with Trystero's own JavaScript API, backed by webcpp::trystero" (section 7). | Trystero shares one `selfId` and one connection per peer across the rooms of a page; webcpp::trystero does so only with the `app` of the shared connection. | The package is built on `app` alone: the shared connection's integration is a precondition of Task 22. The facade gives every app the page's one `selfId`, as Trystero does. |
| R3 | The visualizer's C++ pane shows "the scope that runs", marked with `WEBCPP_SITE_TRACE_SCOPE` (section 6). | The cursors' wasm is trystero's package, a library artifact that knows nothing of the site, and the page should run the very bytes a JavaScript developer gets. | The page runs the package unchanged. Its C++ pane shows the package's `js/bindings.cpp`; its scopes come from the crossings, each a tagged region of that file (`// tag::<name>[]`), mapped by name at build time. The package reports its crossings through `observe`, webcpp's one addition to Trystero's surface, which hands copies, never views on the wasm heap (I4), and which G4 approves by name. |
| R4 | "No server, no runtime API, no cookie. The only third-party scripts are Google's search element" (section 10). | Trystero's examples need its public Nostr relays and the default STUN servers (Google's and Cloudflare's), which see the reader's address, as peers do. | Connections to relays and STUN are the example's purpose, not scripts. No page connects anywhere on load. An example joins only when the reader presses Join, under the notice of G7. Tests and CI reach no public relay (section 11), which a canary in the browser lane proves (Task 29). |
| R5 | "Everything the site serves is built by this command" (section 3), and on the server is "shown as recorded terminal sessions" (section 7). | `b2 site` has neither wasi-sdk nor wasmtime. The served tests run in the site's own lane `http`. | The sessions come from the example's `.expected` transcript, which the served tests check on wasip2 and wasip3 under wasmtime and natively. It is committed, as wasi's are. `b2 site` builds the sessions from it into its build directory. No output is typed. |
| R6 | "On the server: the same program as a WASI component, served by wasmtime, and natively" (section 7). | webcpp::wasi has no native host. | The handler is one function, `answer(method, target)`, returning `webcpp::wasi::http::response`. Two `main`s call it: one between wasi's macros, and one native host of the example's own on Boost.Beast. Both answer the same `.requests` with the same `.expected`. The controller's ruling on Q1 (2026-10-11): no host in webcpp::wasi for now; one comes with webcpp::beast, to which this host moves. |
| R7 | C++ calls the browser needs "the trace and the loader" (section 7). | The owner's decision of 2026-10-10, "webcpp::asio", makes webcpp::asio the way the browser's promises and events reach C++. | The example is a program on webcpp::asio. The controller's ruling on Q2 (2026-10-11), from the owner's decision: no version on `emscripten::val` alone; Task 33 is scheduled last. |
| R8 | The JavaScript API "belongs to trystero ... Its specification is trystero's" (section 7). | No such specification exists. | Task 21 writes `~/.buzz/PLANS/WEBCPP_TRYSTERO_JS_SPEC.md` from this plan's interfaces; G4. |
| R9 | (none) | Task 32 decodes a query with Boost.URL; `/webcpp//boost_url` is not in main yet. | Task 32 links `/webcpp//boost_url` when Task 20 recorded it in `main`; otherwise it compiles `<boost/url/src.hpp>` once, in `examples/on_the_server/boost_url.cpp`, a static library every program of the Jamfile links, removed by the change that brings `/webcpp//boost_url`. |
| R10 | Library documentation is "taught the way the original's documentation teaches, with C++ only in listings" (WEBCPP_DESIGN.md section 5, chapter 8). | A JavaScript face has JavaScript listings. | trystero's AsciiDoc page gains a short chapter, with no listing, which links the site's page "Using a webcpp library from JavaScript" through the attribute `{webcpp-site}`, relative in the site's layout (M5). That page holds the JavaScript, from files the site's tests run. |
| R11 | State machine: "the same XState JSON run by webcpp::xstate in wasm, beside the browser's own state" (section 7). | "The browser's own state" has two readings: React's, or XState JS in the page. The site runs no XState JS. | The page shows, beside the machine diagram, the snapshot the C++ machine returned and the state React holds from it. XState JS stays the oracle of the tests alone. |
| R12 | File transfer "needs trystero's request actions, transfer decisions and progress" (section 7's table). | Trystero's file transfer sketch uses `onReceive`, `onReceiveProgress`, metadata and `onProgress`, no request action. | Recorded for increment 6, to tell the owner when it is planned. |
| R13 | `examples/<name>/` holds "its C++, its page, its explanation" (section 4). | WEBCPP_SITE_PLAN Task 17 puts the pages under `pages/examples/`. | Task 17's shape stands. |
| R14 | The three examples of trystero.dev "become trystero's measure of a complete port" (section 7), while the site's examples live in the site's repository (decision 11). | A library knows nothing of the site. | trystero keeps its own copy of the cursors sketch, which its interop lane runs (Task 24), named in trystero's README as derived. When both are checked out, a site test asserts the two are the same bytes. |
| R15 | The PWA caches every file (section 10). | The program of Task 33 has two variants, of which a browser loads one. | Both are precached. |
| R16 | "The sizes of section 8 are regenerated and compared in a check, as other generated files are" (section 11). | A committed size file couples the site's repository to every measured library and to the build machine: any byte a library's wasm moves turns `main` red until the site regenerates it (I5). | Under O5's recommendation: every `b2 site` measures the sizes into `bin/site/sizes.json`, never committed, and the check holds every number the pages show to that measurement. The spec's sentence would read: "The sizes of section 8 are measured by every build of the site into `bin/site/sizes.json`, and a check holds every number a page shows to that measurement." |

## Owner gates

Outward acts and decisions that are the owner's. The controller resolves each before the task
that needs it; a subagent never performs one. Each carries the controller's recommendation.

- **G2 (as in WEBCPP_SITE_PLAN):** push trystero, the site and the superproject, after each
  increment's review.
- **G4, trystero's JavaScript API.** Recommendation: the owner approves the specification in
  advance, to stand once its independent review is clean, as he did the shared connection's,
  naming the three points he approves: `observe` (webcpp's addition to Trystero's surface,
  handing copies); the refusal list (every member of Trystero 0.26.0's configuration and
  options is implemented or refused with a `TypeError`, none accepted in silence); and the
  delivery of every event on a microtask, a throwing handler reported as Trystero does.
  Needed by Task 22.
- **G5, decided (2026-10-11):** the package is `@webcpp/trystero`, version `0.1.0`,
  `"private": true` until G6. Creating the npm organisation `webcpp` is the owner's own act.
- **G6, publication on npm.** Recommendation: publish only from CI, with npm provenance, under
  the organisation `webcpp` enforcing two-factor authentication, after the package has run on
  the preview through one review cycle; publication runs through a b2 target only. The README
  says the files will change (not the surface) when trystero moves onto webcpp::asio. Until
  then the JavaScript door shows the package as served by the site and built by b2, and a
  page test refuses `npm install @webcpp/trystero` while `"private": true`.
- **G7, the public network from the site.** The options:
  - (a) keep Trystero's app id, `dev.trystero.examples.cursors`, and room `canvas`: the site's
    readers may meet any Trystero JS page running the same sketch;
  - (b) a webcpp app id, a second changed line, which keeps the site's readers among
    themselves;
  - (c) no live join on the preview, only the recorded trace.

  Recommendation: (a), joined only on the reader's gesture, with I1's microtask delivery, I2's
  refusals and Review Focus 1's helpers making a stranger's messages harmless: meeting any
  Trystero JS page is the strongest proof the example can give. The notice above Join, for the
  owner to approve with the gate:

  > Join connects this page to Trystero's public Nostr relays and to public STUN servers, and
  > then directly to the other readers in this room, on this site or on any page running
  > Trystero's own example. They see your IP address, as in any WebRTC call, and your
  > pointer's position while you are joined. Nothing connects until you press Join.

  What each option changes in this plan:
  - (a): nothing; the plan is written for it.
  - (b): Task 28 changes a second line of `cursors.js`, the app id (`'dev.trystero.examples.cursors'`
    becomes `'org.webcpp.examples.cursors'`), and the provenance test, the README and R1 say
    "two lines changed" and check both; the notice drops "or on any page running Trystero's own
    example".
  - (c): Task 28 still builds the frames, the package and the visualizer, but the page carries
    no Join and no notice: the visualizer plays a trace recorded by Task 29's two-page scenario
    over the local relay, committed under the site's `test/fixtures/` and read by the page,
    and the page says that the live example runs from `b2 site-serve`.

  Needed by Task 28 Step 6, and before Task 29 Step 8's push, which publishes the page: the
  push carries the option chosen, never (a) by default.
- **O5, the sizes.** Recommendation: R16; the plan is written for it. If the owner keeps a
  committed file instead, Task 27 changes in three ways: a superproject commit that moves a
  measured library regenerates the site's file in the same superproject commit; the package's
  flags gain `-DBOOST_DISABLE_CURRENT_LOCATION` and a prefix map for Boost's root; and
  determinism is proven between macOS and the CI's Linux.

Settled by the controller on 2026-10-11, recorded here: Q1 (no host in webcpp::wasi for now,
R6), Q2 (no, R7), O3 (the owner's ruling of 2026-10-10 stands; both facts in the README; no
outward question), O4 (`libs/trystero/js/` accepted; recorded in WEBCPP_DESIGN.md by the
controller and in AGENTS.md chapter 1 by Task 25; built in one of trystero's lanes).

## Global Constraints

**Repositories and paths:**
- The superproject is `~/dev/webcpp-site`, the worktree of `~/dev/webcpp` on branch `site`.
  The controller integrates into `main`.
- The site is `~/dev/webcpp-site/site`, branch `main`, the submodule `site`.
- trystero's changes are made in `~/dev/webcpp-site/libs/trystero`, on a branch `js-api`
  from trystero's `main` once the shared connection is in it (Task 20 Step 4). Each library
  commit is followed by a superproject commit of `libs/trystero`, by path.
- webcpp::asio is `libs/asio` once it is a submodule of the superproject's `main`.
- Every b2 command runs from `~/dev/webcpp-site` as `$B2`:

  ```bash
  env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam
  ```

  - A verdict is `-a`.
  - Each lane has its own `--build-dir`.
  - No cap on concurrent builds (owner, 2026-10-11): a test that fails under load is flaky and is fixed, never protected by running less at once.
- The ledger is `~/dev/webcpp/.superpowers/sdd/WEBCPP_SITE_PLAN_4_5/progress.md`.

**Commits and text:**
- Every commit-writing git command is
  `git -c user.name=R -c user.email=github.overboard312@passmail.net commit ...` (and `rebase`,
  `merge` the same way), with no trailer of any kind and never an agent co-author.
- After each commit:
  - `git log -1 --format='%an %ae %cn %ce'` prints
    `R github.overboard312@passmail.net R github.overboard312@passmail.net`;
  - `git log -1 --format=%B | git interpret-trailers --parse` prints nothing.
- Stage by path. Never `git add -A`, `git add .` or `git add libs`.
- Everything written to a file is English. No em dash (U+2014). The word the lint forbids
  (`tools/lint/rules.py`) appears nowhere.
- Every source file starts with the BSL-1.0 notice of webcpp. A file that holds Trystero's own
  code keeps it unchanged instead, with `LICENSE-TRYSTERO.txt` beside it and a README naming
  what was derived (WEBCPP_DESIGN.md section 1, and 2026-10-10, "Trystero's licence").

**The library's rules** (trystero's `AGENTS.md` and the superproject's chapter 6): errors as
values; no static mutable state; no `throw`, `try` or `catch` of the library's own; every
run-time allocation through the user's resource (2026-10-10, "allocators, scope"), proven by a
counting test that sees the global `operator new` and the C library too.

**The site's rules** (WEBCPP_SITE_PLAN's Global Constraints, unchanged, and these):
- **What the site serves comes from `b2 site`.** The package comes in through its b2 target,
  the sessions from committed transcripts the lanes check (R5), and the sizes from the build's
  own measurement (R16).
- **No public network in any build or test.** Chrome runs with
  `--host-resolver-rules=MAP * ~NOTFOUND, EXCLUDE 127.0.0.1`, as the interop lane does, and
  the site's browser lane proves it with a canary (Task 29). Every relay is `relay.mjs` on
  127.0.0.1.
- **No connection on load.** An example that reaches a relay, a STUN server or a peer starts
  only on the reader's gesture, under the notice of G7.
- **The package's name is read, never typed:** from `libs/trystero/js/package.json` in
  trystero; from the copy the site's `packages` target or `dev` places in
  `build/packages/trystero/package.json` (Task 26), in the site.
- **The JavaScript path works in the site alone** (2026-10-10, "two ways into the site's
  repository"). `npm test` and `npm run typecheck` pass without b2: a test or a type check
  that needs the package skips, printing why and how to get it (`npm run dev` once the
  preview serves it, or `b2 site`), and never passes in silence. Under b2 the same checks are
  required: the `packages` target writes `build/packages/required`, and with it a missing
  package fails.
- **Trystero's sketch keeps its bytes but the import line.**
  `test/examples_provenance_test.mjs` holds it to the recorded SHA-256.
- **Mobile first.** Every new page is checked at 360, 768, 1280 and 1440 px, light and dark.
  An example surface that reads the pointer sets `touch-action: none`, so that a finger draws
  instead of scrolling.
- **Every `.wasm` is served as `application/wasm`,** by `serve.mjs`, by the browser lane's
  server and by the service worker's cached responses, so that Emscripten instantiates by
  streaming without a warning (Task 29).

## Review Focus

1. **A peer's malicious or broken cursor message.**
   - Inputs: a JSON `null`; `x` or `y` not a number, NaN, out of range; `color` holding CSS
     or markup; a string of a megabyte; hundreds of peers.
   - Expected: `null` makes the sketch's own handler throw, which the facade reports with
     `console.error('Trystero action handler error:', err)`, as Trystero does, and the next
     message is still delivered; the helpers clamp positions to the surface, accept only a
     `#rrggbb` colour (else the peer's own colour), never write HTML, and draw at most 64
     cursors.
   - Task 23 tests the report; Task 28 tests the helpers.
2. **A browser that cannot run the package.**
   - Inputs: the wasm blocked; an insecure context without `crypto.subtle`; no
     `RTCPeerConnection`.
   - Expected: the frame says so in text, the visualizer says the example could not start and
     why, and no error reaches the console uncaught.
   - Task 23 tests the facade's refusals; Task 29 tests all three in Chrome.
3. **Trystero features webcpp does not have, reached by a user's code.**
   - Inputs: every member of Trystero's configuration and options outside the supported list
     (Task 21), `rtcConfig.iceTransportPolicy: 'relay'` above all, which would otherwise expose
     the address it asked to hide.
   - Expected: each throws a `TypeError` naming it and trystero's roadmap; none is accepted in
     silence.
   - Task 23 tests every member.
4. **A room's life as Trystero JS lives it.**
   - Inputs: `joinRoom` twice for one app and room; `makeAction` twice for one name; `leave`
     then `joinRoom` again; an `onPeerJoin` set after peers joined; a handler that leaves its
     room; `beforeunload`; a send to one joined and one unknown peer; `send(undefined)`.
   - Expected: the same room; the same action; a new room; the handler called with each joined
     peer; the leave after the handler returns; every room left; a warning for the unknown peer
     and the message sent to the other; Trystero's refusal of `undefined`.
   - Task 22 tests the runtime re-entered from its own events; Task 23 the facade; Task 24
     against Trystero JS.
5. **The server and the program's input.**
   - Inputs to the server: an expression that does not parse; an `expr` of 4,097 bytes, one
     of 6,000 bytes (beyond `answer`'s limit and below the native host's header limit), one of
     100 KB; percent-encoded bytes that are not UTF-8; a `POST` with `Content-Length: 0`; an
     unknown path.
   - Expected: the same 4xx, with the same text, from wasmtime (wasip2 and wasip3) and from
     the native host.
   - Inputs to C++ calls the browser: a reader who never answers, gives up, or leaves the page
     while C++ awaits a promise.
   - Expected there: no uncaught error, the program waiting, or ending when the page stops it,
     within a bound.
   - Tasks 32 and 33 test them.

---

## File structure

**trystero** (`libs/trystero`):

| Path | Responsibility |
| --- | --- |
| `include/webcpp/trystero/room.hpp` | gains `raw_payload` and its `payload_traits`: a message's format and bytes, undecoded, for a face in another language |
| `js/runtime.hpp` | `webcpp::trystero::js::runtime<Events>`: the page's apps and rooms by handle, testable natively over the memory backend |
| `js/bindings.cpp` | the embind module over `runtime`, owning the io_context, the event loop and the browser backend; each binding a tagged region |
| `js/facade.mjs` | `createFacade(module, options)`: Trystero's surface over the module |
| `js/index.mjs` | the package's entry: loads the module, exports the facade |
| `js/index.d.mts` | the types, Trystero 0.26.0's for the surface kept |
| `js/package.json`, `js/README.md` | the package's manifest (`@webcpp/trystero`, private until G6) and its README |
| `js/Jamfile` | `trystero` (the program), `flags`, `package` (the directory a page or npm takes) |
| `test/js_runtime_test.cpp`, `test/js_runtime_allocation_test.cpp` | the runtime over the memory backend, and its allocations, native and emscripten |
| `test/js/` | `Jamfile`, `package.json`, `facade_test.mjs`, `fake_module.mjs`: the facade over a fake module (`webcpp.js-test`); the package built in the lane `js` |
| `test/oracle/interop/api.html`, `run.mjs`, `cursors.js`, `cursors_page.html` | the package in the interop lane, Trystero's sketch among its scenarios |
| `doc/javascript.adoc`, `doc/differences.adoc`, `doc/roadmap.adoc`, `doc/testing.adoc`, `README.md` | the page |

**The superproject** (`~/dev/webcpp-site`):

| Path | Responsibility |
| --- | --- |
| `tools/drive/drive.jam`, `tools/drive/drive.py`, `tools/webcpp.jam`, `tools/drive/drive_test.py` | `webcpp.js-test`: a test of b2's that runs `node --test` over a library's JavaScript |
| `tools/component/serve.py`, `component.jam`, `serve_test.py`, `tools/webcpp.jam` | `--native` and `webcpp.serve-native`: a native program served and compared as a component is |
| `tools/webcpp.jam`, `Jamroot`, `tools/lint/compile_commands.py`, `tools/ci/matrix.py`, `.github/workflows/library.yml`, their tests | the site's own lanes, from `site/examples` |
| `tools/site/site.jam`, `tools/ci/assemble.py`, `assemble_test.py` | the package and the sizes into `bin/site` |
| `tools/doc/doc.jam` | the attribute `{webcpp-site}` for the libraries' pages |
| `AGENTS.md` | chapters 1 (a library's `js/`) and 9 (`webcpp.js-test`, `webcpp.serve-native`, the site's lanes) |

**The site** (`~/dev/webcpp-site/site`):

| Path | Responsibility |
| --- | --- |
| `src/components/frame.tsx` | `Frame(title, body)`: the root of an example's surface page |
| `src/components/example_surface.tsx` | `ExampleSurface(src, title, peers)`: the frames of an example in a page |
| `src/components/terminal.tsx` | `Terminal(sessions)`: recorded sessions, replayed |
| `src/components/size_table.tsx` | `SizeTable()`: every measured library, from the build's measurement |
| `src/scopes.ts` | `regionsOf(source)`, `scopesFrom(regions)`, `anchorsOf(source, anchors)`: crossings to C++ regions and JavaScript lines |
| `tools/sizes.mjs` | the measured sizes, written into the build |
| `tools/sessions.mjs` | sessions from a transcript |
| `tools/typecheck.mjs` | the package's type check, run when the package is present |
| `examples/cursors/` | `cursors.js` (Trystero's), `LICENSE-TRYSTERO.txt`, `README.md`, `draw.js`, `surface.ts` |
| `examples/state_machine/` | `Jamfile`, `state_machine.ts` |
| `examples/on_the_server/` | `answer.hpp`, `calculator_http.cpp`, `native_host.cpp`, `calculator_http.requests`, `calculator_http.expected`, `answer_test.cpp`, `Jamfile`, and `boost_url.cpp` under R9's second branch |
| `examples/cpp_calls_browser/` | `guess.cpp`, `page.ts`, `Jamfile` |
| `pages/examples/<name>.oui`, `pages/examples/<name>/explained.oui`, `pages/examples/cursors/surface.oui` | the pages |

---

# Increment 4: trystero's JavaScript API and the collaborative cursors

## Task 20: The ledger, the baseline, the preconditions and the rebase

**Files:**
- Create: `~/dev/webcpp/.superpowers/sdd/WEBCPP_SITE_PLAN_4_5/progress.md`

- [ ] **Step 1:** Create the ledger with the first line
  `# SDD ledger - plan: ~/.buzz/PLANS/WEBCPP_SITE_PLAN_4_5.md`.
- [ ] **Step 2: Record the baseline:**
  - `git -C ~/dev/webcpp-site rev-parse HEAD` and `git -C ~/dev/webcpp-site submodule status`;
  - `git -C ~/dev/webcpp-site/site log --oneline -1`;
  - `git -C ~/dev/webcpp-site/libs/trystero log --oneline -1`.
- [ ] **Step 3: Check each precondition** of the table above and record what each prints.
  - **Site:** `ls site/app site/examples site/machines site/include/webcpp_site`. Every entry
    named by Tasks 13 to 17 is present, and `grep -c 'Task 1[2-8]: complete'` over the site's
    ledger prints 7.
  - **Shared connection:** the shared plan's ledger
    (`~/dev/webcpp/.superpowers/sdd/WEBCPP_TRYSTERO_SHARED_PLAN/progress.md`) records its
    Task 16 complete, and `git -C ~/dev/webcpp log --oneline -20 -- libs/trystero` shows
    `libs/trystero: one connection shared by the rooms of an app`.
  - **asio:** `git -C ~/dev/webcpp log --oneline -- tools/node-test` (its Task 2) and
    `git -C ~/dev/webcpp submodule status libs/asio` (its publication), each possibly absent.
  - **Boost.URL:** `git -C ~/dev/webcpp grep -n "boost_url" main -- Jamroot`. Record "R9: link
    `/webcpp//boost_url`" when it prints a `lib boost_url` line, else "R9: compile
    `<boost/url/src.hpp>`". Task 32 reads this record.
  - **Gates:** G4, G7 and O5, each resolved or not, from the controller's entries.
- [ ] **Step 4: Bring the shared connection in (when its Task 16 is integrated).**

```bash
cd ~/dev/webcpp-site
git fetch origin
git -c user.name=R -c user.email=github.overboard312@passmail.net rebase origin/main
git submodule update --init libs/trystero
git -C libs/trystero fetch origin
git -C libs/trystero switch -c js-api origin/main
```

  Expected:
  - the rebase ends without a conflict, or each conflict is resolved and recorded in the
    ledger, as the earlier integration recorded its two;
  - `git -C libs/trystero log --oneline -1` is the commit the superproject's `main` points
    to, and `grep -n "class app\b" libs/trystero/include/webcpp/trystero/*.hpp` finds `app`;
  - `grep -rn "room_config" libs/trystero/include` finds nothing;
  - `$B2 -a --build-dir=bin/b-site site` is green, as before the rebase;
  - `git log --format='%an %ae %cn %ce' origin/main..HEAD` prints only
    `R github.overboard312@passmail.net R github.overboard312@passmail.net`.
- [ ] **Step 5:** A precondition that is not met blocks only the tasks that need it (the
  table's last column). Record which tasks may start, and stop there for the controller.

## Task 21: The specification of trystero's JavaScript API (gate G4)

**Files:**
- Create: `~/.buzz/PLANS/WEBCPP_TRYSTERO_JS_SPEC.md`

**Interfaces:**
- Produces the specification Tasks 22 to 25 implement. It is this plan's proposal, below,
  written out with its reasons and Trystero 0.26.0's sources (`C/` as the shared connection's
  specification cites them).

**The proposal the specification writes out:**

- **The package.**
  - **Where:** `libs/trystero/js/`, a library's JavaScript face (O4), built by
    `b2 toolset=emscripten libs/trystero/js//package`, in the release variant, into
    `<build>/package/`. Its files:
    - `index.mjs`, `facade.mjs`, `index.d.mts`;
    - the module's glue `trystero.js` and `trystero.wasm`;
    - `package.json`, `README.md`, `LICENSE_1_0.txt`, `LICENSE-TRYSTERO.txt`.
  - **Name and version:** `@webcpp/trystero`, `0.1.0`, `"private": true` until G6. It is ESM
    only, `"type": "module"`, and `"sideEffects"` covers `index.mjs` alone.
  - **Where it runs:** a browser's window, in a secure context, with WebRTC. It is linked with
    `-sENVIRONMENT=web`, so node and workers, where Trystero JS runs, are not supported: a
    difference (M13).
  - **Loading:** `index.mjs` awaits the module at its top level, so that an `import` of the
    package resolves once the wasm is compiled, and `joinRoom` is synchronous as Trystero's.
- **The exports, Trystero's:**
  - `selfId`: 20 characters of Trystero's 62-character set, drawn once at load from
    `crypto.getRandomValues`. Trystero uses `Math.random`; the alphabet and length are the
    same.
  - `joinRoom(config, roomId, callbacks?) -> Room`:
    - one `app` per `config.appId`, made at that app's first join, with `self_id` = `selfId`;
    - the app's settings come from the first config of that `appId`, as Trystero's relays do
      (`C/strategy.mjs:131-136`): `relayConfig.urls`, `relayConfig.redundancy` and
      `rtcConfig.iceServers`;
    - the room's settings come from each call: `password`, `maxReceiveBytes` and
      `callbacks.handshakeTimeoutMs`;
    - a second `joinRoom` for an app and room still joined returns the same `Room`, as Trystero
      does (`C/strategy.mjs:37`). The runtime is never asked twice, so `error::already_joined`
      never reaches JavaScript;
    - a 65th room of one app throws an `Error` with `code` `too_many_rooms`
      (`app::max_rooms`, 64), which Trystero lacks: a difference (M13);
    - `callbacks.onJoinError({error, appId, roomId, peerId})` is called for a room's
      `error::wrong_password`, with Trystero's text.
  - **`Room`:**
    - `makeAction(namespace, config?)` returns `{send, onMessage, onReceiveProgress,
      onReceive}`; a second call with the same namespace returns the same action, as
      Trystero's (`C/actions.mjs:105-106`); `config.onMessage` sets the action's first
      handler;
    - `ping(id) -> Promise<number>`;
    - `leave() -> Promise<void>`, after which `joinRoom` makes a new room;
    - `isPassive() -> false`;
    - `getPeers()`: a record whose keys are the joined peers and whose values are `null`, a
      difference (`differences.adoc`, "`peers()` lists ids");
    - `onPeerJoin` and `onPeerLeave`, as properties. Setting `onPeerJoin` calls it at once,
      synchronously inside the setter, with each peer already joined, from `peers()`, as
      Trystero's setter does (`C/room.mjs:216-218`). This replay is not deferred: the setter
      runs in the user's code, never inside the wasm's call, so it cannot re-enter the runtime;
      a handler that throws there throws out of the setter, as in Trystero.
  - **`MessageAction.send(data, options?)`:**
    - `undefined` data rejects with Trystero's error, "action data cannot be undefined"
      (`C/action-wire.mjs:128`);
    - text, JSON or binary as Trystero's `C/action-wire.mjs:129-133` decides: a `Blob`,
      `ArrayBuffer` or view is binary (a `Blob` read with `arrayBuffer()`), a string is text,
      anything else is JSON (`JSON.stringify`);
    - `options.target` (a string, an array or null) and `options.metadata` (JSON);
    - the targets are checked against `peers()` first: each one not joined warns
      `Trystero: no peer with id <id> found`, and the message goes to the others, as
      Trystero's `iterate` does (`C/room.mjs:28-35`); with none left, the promise resolves
      (M2);
    - every other failure rejects with an `Error` whose `message` is the C++ error's message
      and whose `code` is its name.
  - **Delivery (I1):** every event the runtime reports is copied out of the wasm memory
    first, then delivered to the user's handler on a microtask, never from inside the wasm's
    call. A handler that throws is reported with
    `console.error('Trystero action handler error:', err)`, as Trystero's
    `C/actions.mjs:134-138` does for `onMessage` (and with the same words for the room's
    handlers), and the next event is still delivered. So a handler may call `leave` or `send`
    at once: the runtime is never re-entered from its own event.
  - **Received messages:** text arrives as a string, JSON as `JSON.parse` of the bytes (a
    message whose JSON does not parse is dropped with a warning), binary as a `Uint8Array`
    (Trystero 0.26 gives a `Uint8Array`), with the context `{peerId, metadata}`.
  - **Leaving:** every room leaves on `beforeunload`, as Trystero's do.
- **Every configuration member, supported or refused (I2).** The specification has one table
  per type of Trystero 0.26.0's `types.d.mts`, each member supported or refused, and a member
  of no table (a key Trystero does not have either) is refused too. Refusing is a `TypeError`
  whose message names the member and says "is not supported by webcpp's trystero yet; see its
  roadmap". The proposal:

| Type | Supported | Refused |
| --- | --- | --- |
| `BaseRoomConfig` | `appId`; `password`; `maxReceiveBytes`; `passive: false`; `trickleIce: true` | `passive: true`; `trickleIce: false`; `turnConfig`; `rtcPolyfill`; every `_test_only_*` |
| `RelayConfig` (`relayConfig`) | `urls`; `redundancy`; `manualReconnection: false`; `warnOnRelayFailure: false` | `manualReconnection: true`; `warnOnRelayFailure: true` |
| `RTCConfiguration` (`rtcConfig`) | `iceServers`; `iceTransportPolicy: 'all'` | `iceTransportPolicy: 'relay'` (above all: ignoring it would expose the address it asks to hide); `bundlePolicy`, `rtcpMuxPolicy`, `certificates`, `iceCandidatePoolSize` and any other member |
| `JoinRoomCallbacks` | `onJoinError`; `handshakeTimeoutMs` | `onPeerHandshake` |
| `MessageActionConfig` | `kind: 'message'`; `onMessage` | `kind: 'request'`; `onReceiveProgress`; `onReceive` |
| `SendOptions` | `target`; `metadata` | `onProgress`; `signal` |
| `MessageAction` properties | `onMessage` | a non-null `onReceiveProgress` or `onReceive` |
| `Room` members | as above | `addStream`, `removeStream`, `addTrack`, `removeTrack`, `replaceTrack`; a non-null `onPeerStream` or `onPeerTrack` |
| exports | `selfId`, `joinRoom` | `getRelaySockets`, `pauseRelayReconnection`, `resumeRelayReconnection`, `defaultRelayUrls` |

- **Where the page cannot run it (M17):** without `crypto.subtle` (an insecure context) or
  without `RTCPeerConnection`, `joinRoom` throws an `Error` saying what is missing, before any
  room is made.
- **webcpp's one addition:** `observe(observer | null)`, where `observer = {call(name, args),
  result(name, value), callback(name, args)}`.
  - It is called on each crossing between the facade and the wasm: `call` and `result` around
    every binding the facade calls, `callback` for every event the runtime reports.
  - `name` is the binding's (`Runtime.join`, `Runtime.send`, ..., `events.message`).
  - **Its arguments are copies (I4):** strings, numbers and plain objects; a payload is
    summarised as `{format, length}` plus, for text and JSON, its decoded value; never a view
    on the wasm memory, never an `ArrayBuffer` beyond the payload's own bytes.
  - `null` removes it. An observer that throws is detached and the error is reported with
    `reportError`, never thrown into the room.
- **trystero's addition:** `raw_payload`, an action type that hands a message's format and
  bytes undecoded, and sends bytes with the format given. Without it, a JSON message would be
  parsed in C++ and serialized again, which can change its text (`1.0` becomes `1`).
- **Differences, on trystero's page:** `getPeers`' values; `selfId` from
  `crypto.getRandomValues`; `observe`; the browser's window alone (`-sENVIRONMENT=web`);
  `max_rooms`; every refused member above.
- **Tests:**
  - the runtime natively and on emscripten over the memory backend, and its allocations;
  - the facade under node over a fake module;
  - the package, in its release bytes, against Trystero JS 0.26.0 in the interop lane, the
    cursors sketch among its scenarios.
- **Sizes:** the package's files beside Trystero 0.26.0 bundled with esbuild (site spec
  section 8).

- [ ] **Step 1:** Write the specification, with:
  - section 1, purpose and the owner's decisions;
  - section 2, what Trystero 0.26.0 does, cited in `dist`;
  - section 3, the package;
  - section 4, the surface, member by member, each supported or refused, with the tables;
  - section 5, delivery on a microtask;
  - section 6, the C++ runtime and `raw_payload`;
  - section 7, `observe`;
  - section 8, differences;
  - section 9, tests;
  - section 10, sizes;
  - section 11, the increments, which are Tasks 22 to 25 of this plan;
  - section 12, the owner's points: G4's three (`observe`, the refusal list, the microtask
    delivery) and G6.

  Cite each Trystero behaviour by file and line in the pinned `dist`, as the shared
  connection's specification does.
- [ ] **Step 2:** The controller dispatches its independent review (most capable model) with
  this plan's Review Focus. Fix each finding; a finding that changes an owner's decision goes to
  the owner.
- [ ] **Step 3 (controller, G4):** resolve G4 as its recommendation says. Record it in the
  ledger, and in `WEBCPP_DESIGN.md`'s owner decisions with O4 (the controller writes there,
  never a subagent). If the approved specification differs from this plan's interfaces, the
  controller amends Tasks 22 to 25 before dispatching them.

## Task 22: `raw_payload` and the runtime (trystero)

**Files:**
- Modify: `libs/trystero/include/webcpp/trystero/room.hpp` (`raw_payload`, its
  `payload_traits`, `make_action`'s list of types).
- Create:
  - `libs/trystero/js/runtime.hpp`;
  - `libs/trystero/test/js_runtime_test.cpp`;
  - `libs/trystero/test/js_runtime_allocation_test.cpp`;
  - `libs/trystero/test/raw_payload_test.cpp`.
- Modify: `libs/trystero/test/Jamfile` (the `webcpp.run` lines and their `_no_exceptions`
  twins), `libs/trystero/doc/usage.adoc` (the types an action takes).

**Interfaces:**
- Consumes: `app`, `app_config`, `room_options`, `app::join`
  (WEBCPP_TRYSTERO_SHARED_SPEC.md section 4.5); `action<T>`, `send_options`, `message_info`
  (`room.hpp`); `test/support/memory_backend.hpp`; the counting resources of
  `test/allocation_test.cpp`.
- Produces (`room.hpp`, public, fully documented):

```cpp
namespace webcpp::trystero {
/** A message as it travels: its format and its bytes, undecoded, for a face in another language. */
struct raw_payload {
    /** Text, JSON or binary, as the frame says. */
    protocol::payload_format format = protocol::payload_format::binary;
    /** The payload's bytes: UTF-8 for text and JSON, as sent. */
    pmr_bytes bytes;
};
}
```

- Produces (`js/runtime.hpp`, the package's own, not installed, namespace
  `webcpp::trystero::js`):

```cpp
/** A room as the page names it: a number never reused by one runtime. */
using room_handle = std::uint32_t;

/** What a page asks to join: the app's settings count at the app's first join only. */
struct join_request {
    std::string_view app_id;
    std::string_view room_id;
    std::string_view password = {};
    std::size_t max_receive_bytes = std::size_t{256} * 1024 * 1024;
    std::chrono::milliseconds handshake_timeout{10'000};
    std::span<const std::string_view> relay_urls = {};
    std::size_t relay_redundancy = 5;
    std::span<const backend::ice_server> ice_servers = {};
};

/**
 * The page's rooms: one app per app id, all with one self id, and each room by its handle.
 *
 * Events is told, on the executor, what happens:
 * peer_join(room_handle, std::string_view peer), peer_leave(room_handle, std::string_view peer),
 * message(room_handle, std::string_view action, std::string_view peer, const raw_payload&,
 *         std::string_view metadata),
 * room_error(room_handle, std::string_view peer, boost::system::error_code, std::string_view),
 * app_error(std::string_view app_id, boost::system::error_code, std::string_view),
 * settled(std::uint32_t ticket, boost::system::error_code, double value).
 * An event handler may call any member, leave included, for the room it is told about.
 */
template <class Events>
class runtime {
public:
    runtime(boost::asio::any_io_executor ex, backend::factory& factory, std::string_view self_id,
            Events& events, std::pmr::memory_resource* memory = std::pmr::get_default_resource());
    [[nodiscard]] std::string_view self_id() const;
    /** Joins, making the app at its first room: error::too_many_rooms, error::already_joined. */
    boost::system::result<room_handle> join(const join_request& request);
    /** Makes the action `name` of a room; error::invalid_action_name for a bad name. */
    boost::system::error_code make_action(room_handle room, std::string_view name);
    /** Sends; settled(ticket, ec, 0) when it completes. */
    void send(room_handle room, std::string_view action, raw_payload payload,
              send_options options, std::uint32_t ticket);
    /** settled(ticket, ec, milliseconds). */
    void ping(room_handle room, std::string_view peer, std::uint32_t ticket);
    /** settled(ticket, ec, 0); the handle is then unknown. */
    void leave(room_handle room, std::uint32_t ticket);
    [[nodiscard]] std::pmr::vector<peer_id> peers(room_handle room) const;
};
```

  An unknown handle makes `make_action` return `error::room_left`, and makes `send`, `ping` and
  `leave` settle with it. `join_request` holds views, copied into `memory` at the call, so that
  no default-heap container is made from it (I12).

- [ ] **Step 1: Write the failing tests.**
  - **`test/raw_payload_test.cpp`** (`webcpp.run raw_payload : raw_payload_test.cpp ;`), two
    rooms of two apps over the memory backend, as `room_test.cpp` makes them:
    - a JSON text `{"a":1.0}` sent as `raw_payload{json, bytes}` arrives as the same bytes with
      format json at a `raw_payload` action, and as `boost::json::value` at a
      `boost::json::value` action of the same name;
    - a text and a binary payload keep their format and bytes;
    - a `raw_payload` of format json whose bytes are not JSON is delivered as sent, since raw
      means undecoded; a `boost::json::value` action of the same name receives nothing of it,
      as a message of another format is not delivered to it. The facade, which writes JSON
      only through `JSON.stringify`, drops such a message on receipt with a warning
      (Task 23).
  - **`test/js_runtime_test.cpp`** (`webcpp.run js_runtime : js_runtime_test.cpp : <include>../js ;`),
    a `recording_events` struct that appends each call to a vector, two runtimes on one memory
    network:

```cpp
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The package's runtime over the memory backend: apps by app id, one self id, rooms by handle.
#include "runtime.hpp"
#include "support/memory_backend.hpp"

#include <boost/core/lightweight_test.hpp>

// Cases, each a function called from main:
// - two_rooms_of_one_app_share_the_self_id: a.join({app, "r1"}), a.join({app, "r2"});
//   both handles differ; a.self_id() is the one given; b joins r1 and r2; each side
//   records peer_join for both rooms, with the same peer id.
// - a_second_app_has_the_same_self_id: a.join({"app2", "r"}) meets b's app2 with a's id.
// - the_app_settings_of_a_later_join_are_ignored: a second join of the app naming other
//   relay_urls keeps the first's (relays of the memory network record the URLs used).
// - messages_keep_their_format: send json, text and binary raw_payloads with metadata;
//   message() records action, peer, format, bytes and metadata unchanged.
// - an_unknown_handle_settles_with_room_left: send, ping and leave on handle 999.
// - leave_settles_and_forgets_the_handle: leave(r, 7) settles 7 with success; peers(r) is
//   empty; make_action(r, "x") is error::room_left.
// - leaving_from_its_own_message_event: the events' message() calls leave(room, 9) for the
//   room it is told about; the message is recorded once, 9 settles with success, a second
//   message to that room is not delivered, and the other room of the app goes on (I1).
// - leaving_from_its_own_peer_join_event: the same from peer_join().
// - the_65th_room_is_refused: 64 joins succeed, the 65th is error::too_many_rooms.
// - a_wrong_password_is_a_room_error: a joins with "p", b with "q"; room_error records
//   error::wrong_password on each side, with the other's id.
// - ping_settles_with_milliseconds: settled(ticket, success, value >= 0).
```

  - **`test/js_runtime_allocation_test.cpp`** (I12), modelled on `allocation_test.cpp`: two
    runtimes, each given a counting resource, join two rooms of one app and exchange a text, a
    JSON and a binary payload of 100,000 bytes with metadata, ping, and leave. Every
    allocation goes through the given resources; the global `operator new` and the C
    library's `malloc`, `calloc` and `realloc`, counted as that test counts them, see none
    between the first `join` and the last `settled`.
- [ ] **Step 2:** Run each and watch it fail:

```bash
$B2 -a --build-dir=bin/b-native libs/trystero/test//raw_payload libs/trystero/test//js_runtime libs/trystero/test//js_runtime_allocation
```

  Expected: FAIL, `raw_payload` and `runtime.hpp` unknown.
- [ ] **Step 3: Implement `raw_payload`'s traits.** Encoding takes the format and moves the
  bytes. Decoding moves the frame's bytes into a `pmr_bytes` of the room's resource, with
  the frame's format. Add it to `make_action`'s `@tparam` list.
- [ ] **Step 4: Implement `runtime`.**
  - A `std::pmr::map` from app id to `app`, and from handle to a record holding the room, its
    actions by name (`action<raw_payload>`) and the app id, all in `memory`.
  - Handles count from 1. A handle is removed only after the event that leaves it returns, so
    an event handler may leave its own room.
  - No static state; no `throw`, `try` or `catch` (the library's rules). Each function starts
    with its guards.
- [ ] **Step 5:** Run the three on both targets, with and without exceptions, as trystero's
  `_no_exceptions` tests are declared (`<exception-handling>off` twins in `test/Jamfile`):

```bash
$B2 -a --build-dir=bin/b-native libs/trystero/test//raw_payload libs/trystero/test//js_runtime libs/trystero/test//js_runtime_allocation
$B2 -a --build-dir=bin/b-native-noexc exception-handling=off libs/trystero/test//raw_payload libs/trystero/test//js_runtime libs/trystero/test//js_runtime_allocation
$B2 -a --build-dir=bin/b-em toolset=emscripten libs/trystero/test//raw_payload libs/trystero/test//js_runtime libs/trystero/test//js_runtime_allocation
$B2 -a --build-dir=bin/b-em-noexc toolset=emscripten exception-handling=off libs/trystero/test//raw_payload libs/trystero/test//js_runtime libs/trystero/test//js_runtime_allocation
```

  Expected: PASS, every case, in all four.
- [ ] **Step 6:** Run trystero's lint and doc (`$B2 -a --build-dir=bin/b-doc libs/trystero/doc`).
  Expected: green; `raw_payload` documented in the reference.
- [ ] **Step 7: Commit** trystero, then the superproject pointer:
  - trystero: `"trystero: raw_payload, a message undecoded; the JavaScript package's runtime over app, its allocations counted"`;
  - superproject: `"libs/trystero: raw_payload and the package's runtime"`.

## Task 23: The package: bindings, facade, and `webcpp.js-test`

**Files:**
- Create (trystero):
  - `js/bindings.cpp`, `js/facade.mjs`, `js/index.mjs`, `js/index.d.mts`, `js/package.json`,
    `js/README.md`, `js/Jamfile`;
  - `test/js/Jamfile`, `test/js/package.json`, `test/js/package-lock.json`,
    `test/js/facade_test.mjs`, `test/js/fake_module.mjs`.
- Modify (superproject): `tools/drive/drive.jam`, `tools/drive/drive.py`, `tools/webcpp.jam`
  (`rule js-test`), `tools/drive/drive_test.py`, `AGENTS.md` chapter 9.

**Interfaces:**
- Consumes: Task 22's `runtime` and `raw_payload`; `browser::event_loop`, `browser::backend`
  (`include/webcpp/trystero/browser/`); asio's Task 2 in `main` (precondition, I8).
- Produces:
  - **The embind module `createTrysteroModule`,** class `Runtime`:
    - `new Runtime(selfId: string, events: {peerJoin, peerLeave, message, roomError,
      appError, settled})`;
    - `join(request: object) -> {room: number} | {code: string, message: string}`;
    - `makeAction(room: number, name: string) -> '' | code`;
    - `send(room, action, format: 0|1|2, bytes: Uint8Array, targets: string[], metadata:
      string, ticket: number)`;
    - `ping(room, peer, ticket)`, `leave(room, ticket)`;
    - `peers(room) -> string[]`;
    - `delete()`.
    - Each is a region `// tag::<Class.member>[]` ... `// end::<Class.member>[]` of
      `bindings.cpp`, such as `tag::Runtime.send[]`, and each event callback is
      `tag::events.<name>[]`.
  - **`js/facade.mjs`:**
    `export function createFacade(module, {random, warn, error, addEventListener, secure,
    webrtc, queue}) -> {selfId, joinRoom, observe, getRelaySockets, pauseRelayReconnection,
    resumeRelayReconnection}`.
    - `secure` is false where `crypto.subtle` is missing and `webrtc` false where
      `RTCPeerConnection` is; `joinRoom` then throws.
    - `queue(f)` runs `f` on a microtask (`queueMicrotask` in `index.mjs`); `error` is
      `console.error`.
  - **`test/js/fake_module.mjs`:** `fakeModule()` returns a stand-in of the embind module:
    - its `Runtime` records each call in `calls` (`{name, ...arguments}`);
    - `join` returns `{room: n}`, counting from 1, and sets `lastRoom`;
    - `lastCall(name)`, and `nextCall(name)`, a promise of the next such call;
    - `settleLast(code, message?)` settles the last pending ticket (`''` for success);
    - `fire(event, ...args)` calls the `events` object the facade gave `Runtime`; a `message`'s
      bytes are given as a view on a fake 1 MiB `ArrayBuffer`, as embind would give a view on
      the wasm memory;
    - `peersOf`, a function the test replaces, answers `peers`.
  - **`js/index.mjs`:**

```js
import createTrysteroModule from './trystero.js'
import {createFacade} from './facade.mjs'

const module = await createTrysteroModule()
export const {selfId, joinRoom, observe, getRelaySockets, pauseRelayReconnection,
              resumeRelayReconnection} = createFacade(module, {
    random: n => crypto.getRandomValues(new Uint8Array(n)),
    warn: (...args) => console.warn(...args),
    error: (...args) => console.error(...args),
    addEventListener: (type, f) => globalThis.addEventListener?.(type, f),
    secure: Boolean(globalThis.crypto?.subtle),
    webrtc: typeof globalThis.RTCPeerConnection === 'function',
    queue: f => queueMicrotask(f),
})
```

  - **`js/Jamfile`:**
    - `alias flags`, whose usage requirements are:
      - `<linkflags>-sMODULARIZE`, `<linkflags>-sEXPORT_ES6`,
        `<linkflags>-sEXPORT_NAME=createTrysteroModule`;
      - `<linkflags>-sENVIRONMENT=web`, `<linkflags>-sALLOW_MEMORY_GROWTH`,
        `<linkflags>-lembind`;
      - `<variant>release`;
    - `webcpp.link trystero : bindings.cpp : <library>/webcpp/trystero//trystero <library>flags
      : emscripten`;
    - `make package.stamp`, which copies the program's `.js` and `.wasm`, `index.mjs`,
      `facade.mjs`, `index.d.mts`, `package.json`, `README.md` and the two licences into
      `package/` beside the stamp, and prints that directory. It runs on emscripten alone and
      stops the build, saying so, on another toolset, as `example/chat`'s `site` does.
  - **`test/js/Jamfile`** puts the package in a lane (M12):

```jam
import webcpp ;
webcpp.targets emscripten ;
webcpp.original node ;
webcpp.js-test facade : facade_test.mjs ;
alias package : /webcpp/trystero/js//package ;
webcpp.lane js : facade package : emscripten ;
```

  - **`webcpp.js-test name : files + : targets *`** (superproject; named apart from asio's
    `/webcpp//node-test`, the C++ test support under node, I8): a test of b2's, of the type
    `DRIVE`.
    - It runs `node --test --test-timeout=60000 --test-reporter=tap <files>` with the
      Jamfile's `webcpp.original node` packages, through `drive.py`, which bounds it and stops
      its process group as for a driven test.
    - It fails when the TAP summary reports `# tests 0`, so a lane never passes on nothing.
    - It calls `runs-original`, so the lane is of the kind `original` and the CI sets up node
      for it.

- [ ] **Step 1: Write the failing test of the rule** in `tools/drive/drive_test.py`, in
  its fixture style:
  - a fixture Jamfile with `webcpp.original node ; webcpp.js-test pass : pass_test.mjs ;`
    passes;
  - one whose test asserts false fails, naming it;
  - one whose file holds no test fails, naming "no test ran";
  - one whose test waits for ever is stopped by `--test-timeout` or, past it, by
    `<webcpp-drive-timeout>2`, with no process left;
  - `b2 declared-lanes` prints its lane with the kind `original`.

  Run `python3 tools/drive/drive_test.py`. Expected: FAIL, `js-test` unknown.
- [ ] **Step 2: Implement `webcpp.js-test`** in `drive.jam`, reusing `drive.py` (its command
  `node --test ...`, and its zero-test check on the TAP output). Add the line in
  `tools/webcpp.jam` and a sentence in AGENTS.md chapter 9, beside asio's `/webcpp//node-test`,
  saying what each is for. Run the test. Expected: PASS.
- [ ] **Step 3: Write the facade's failing tests,** `test/js/facade_test.mjs`, over
  `fake_module.mjs`:

```js
// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

import assert from 'node:assert/strict'
import test from 'node:test'
import {createFacade} from '../../js/facade.mjs'
import {fakeModule} from './fake_module.mjs'

const make = ({secure = true, webrtc = true} = {}) => {
    const module = fakeModule()
    const warnings = []
    const errors = []
    const listeners = {}
    const facade = createFacade(module, {
        random: n => new Uint8Array(n).fill(7),
        warn: (...a) => warnings.push(a.join(' ')),
        error: (...a) => errors.push(a),
        addEventListener: (type, f) => (listeners[type] = f),
        secure,
        webrtc,
        queue: f => queueMicrotask(f),
    })
    return {module, facade, warnings, errors, listeners}
}
const tick = () => new Promise(resolve => setTimeout(resolve, 0))

test('an insecure context, or no WebRTC, refuses joinRoom before any room is made', () => {
    for (const options of [{secure: false}, {webrtc: false}]) {
        const {facade, module} = make(options)
        assert.throws(() => facade.joinRoom({appId: 'app'}, 'r'), /secure context|WebRTC/)
        assert.equal(module.calls.length, 0)
    }
})

test('selfId is 20 characters of Trystero\'s set', () => {
    assert.match(make().facade.selfId, /^[0-9A-Za-z]{20}$/)
})

test('joinRoom twice returns the same room; after leave, a new one', async () => {
    const {facade, module} = make()
    const a = facade.joinRoom({appId: 'app'}, 'r')
    assert.equal(facade.joinRoom({appId: 'app'}, 'r'), a)
    const left = a.leave()
    module.settleLast('')
    await left
    assert.notEqual(facade.joinRoom({appId: 'app'}, 'r'), a)
    assert.equal(module.calls.filter(c => c.name === 'join').length, 2)
})

test('makeAction twice returns the same action; config.onMessage is its handler', async () => {
    const {facade, module} = make()
    const room = facade.joinRoom({appId: 'app'}, 'r')
    const got = []
    const action = room.makeAction('x', {onMessage: data => got.push(data)})
    assert.equal(room.makeAction('x'), action)
    module.fire('message', module.lastRoom, 'x', 'peer', 0, new TextEncoder().encode('hi'), '')
    await tick()
    assert.deepEqual(got, ['hi'])
})

test('send chooses Trystero\'s format, and refuses undefined', async () => {
    const {facade, module} = make()
    const action = facade.joinRoom({appId: 'app'}, 'r').makeAction('x')
    for (const [data, format] of [['hi', 0], [{a: 1}, 1], [42, 1], [new Uint8Array([1, 2]), 2],
                                  [new ArrayBuffer(3), 2], [new Blob(['ab']), 2]]) {
        const sent = action.send(data)
        await module.nextCall('send')
        assert.equal(module.lastCall('send').format, format)
        module.settleLast('')
        await sent
    }
    await assert.rejects(action.send(undefined), /action data cannot be undefined/)
})

test('a message is delivered on a microtask, decoded, with its context', async () => {
    const {facade, module} = make()
    const action = facade.joinRoom({appId: 'app'}, 'r').makeAction('x')
    const got = []
    action.onMessage = (data, context) => got.push([data, context])
    module.fire('message', module.lastRoom, 'x', 'peer', 1, new TextEncoder().encode('{"a":1}'), '{"m":2}')
    module.fire('message', module.lastRoom, 'x', 'peer', 0, new TextEncoder().encode('hi'), '')
    assert.deepEqual(got, [])
    await tick()
    assert.deepEqual(got, [[{a: 1}, {peerId: 'peer', metadata: {m: 2}}], ['hi', {peerId: 'peer'}]])
})

test('a handler that throws is reported as Trystero does; the next message still arrives', async () => {
    const {facade, module, errors} = make()
    const action = facade.joinRoom({appId: 'app'}, 'r').makeAction('cursor')
    const got = []
    // The cursors sketch's handler, which destructures its data: a JSON null throws.
    action.onMessage = ({x, y, color}, {peerId}) => got.push([peerId, x, y, color])
    module.fire('message', module.lastRoom, 'cursor', 'p', 1, new TextEncoder().encode('null'), '')
    module.fire('message', module.lastRoom, 'cursor', 'p', 1, new TextEncoder().encode('{"x":0.5,"y":0.5,"color":"#123456"}'), '')
    await tick()
    assert.equal(errors.length, 1)
    assert.equal(errors[0][0], 'Trystero action handler error:')
    assert.deepEqual(got, [['p', 0.5, 0.5, '#123456']])
})

test('a handler may leave its room: the leave reaches the runtime after the event returns', async () => {
    const {facade, module} = make()
    const room = facade.joinRoom({appId: 'app'}, 'r')
    room.makeAction('x').onMessage = () => room.leave()
    module.fire('message', module.lastRoom, 'x', 'peer', 0, new TextEncoder().encode('hi'), '')
    assert.equal(module.calls.filter(c => c.name === 'leave').length, 0)
    await tick()
    assert.equal(module.calls.filter(c => c.name === 'leave').length, 1)
})

test('onPeerJoin set late is called at once, in the setter, with each joined peer', () => {
    const {facade, module} = make()
    const room = facade.joinRoom({appId: 'app'}, 'r')
    module.peersOf = () => ['p1', 'p2']
    const seen = []
    room.onPeerJoin = id => seen.push(id)
    assert.deepEqual(seen, ['p1', 'p2'])
})

test('a send to one joined and one unknown peer warns for the unknown and sends to the other', async () => {
    const {facade, module, warnings} = make()
    const action = facade.joinRoom({appId: 'app'}, 'r').makeAction('x')
    module.peersOf = () => ['p1']
    const sent = action.send('hi', {target: ['p1', 'ghost']})
    await module.nextCall('send')
    assert.deepEqual(module.lastCall('send').targets, ['p1'])
    module.settleLast('')
    await sent
    assert.deepEqual(warnings, ['Trystero: no peer with id ghost found'])
    assert.equal(await action.send('hi', {target: 'ghost'}), undefined)
})

test('every refused member throws a TypeError naming it', async () => {
    const {facade} = make()
    let n = 0
    const room = () => facade.joinRoom({appId: 'refusals'}, `r${n++}`)
    const refused = [
        () => facade.joinRoom({appId: 'a', passive: true}, 'r'),
        () => facade.joinRoom({appId: 'a', trickleIce: false}, 'r'),
        () => facade.joinRoom({appId: 'a', turnConfig: []}, 'r'),
        () => facade.joinRoom({appId: 'a', rtcPolyfill: class {}}, 'r'),
        () => facade.joinRoom({appId: 'a', _test_only_sharedPeerIdleMs: 1}, 'r'),
        () => facade.joinRoom({appId: 'a', relayConfig: {manualReconnection: true}}, 'r'),
        () => facade.joinRoom({appId: 'a', relayConfig: {warnOnRelayFailure: true}}, 'r'),
        () => facade.joinRoom({appId: 'a', rtcConfig: {iceTransportPolicy: 'relay'}}, 'r'),
        () => facade.joinRoom({appId: 'a', rtcConfig: {bundlePolicy: 'max-bundle'}}, 'r'),
        () => facade.joinRoom({appId: 'a', somethingElse: 1}, 'r'),
        () => facade.joinRoom({appId: 'a'}, 'r', {onPeerHandshake: async () => {}}),
        () => room().makeAction('q', {kind: 'request'}),
        () => room().makeAction('q', {onReceive: () => true}),
        () => room().makeAction('q', {onReceiveProgress: () => {}}),
        () => { room().makeAction('q').onReceive = () => true },
        () => { room().makeAction('q').onReceiveProgress = () => {} },
        () => room().addStream({}),
        () => room().removeStream({}),
        () => room().addTrack({}, {}),
        () => room().removeTrack({}),
        () => room().replaceTrack({}, {}),
        () => { room().onPeerStream = () => {} },
        () => { room().onPeerTrack = () => {} },
        () => facade.getRelaySockets(),
        () => facade.pauseRelayReconnection(),
        () => facade.resumeRelayReconnection(),
    ]
    for (const call of refused) {
        assert.throws(call, e => e instanceof TypeError && /is not supported by webcpp's trystero yet/.test(e.message), String(call))
    }
    // send is async, as Trystero's: its refusals are rejections.
    for (const options of [{onProgress: () => {}}, {signal: new AbortController().signal}]) {
        await assert.rejects(room().makeAction('q').send('x', options),
            e => e instanceof TypeError && /is not supported by webcpp's trystero yet/.test(e.message))
    }
    // The defaults Trystero itself takes are accepted.
    assert.doesNotThrow(() => facade.joinRoom({appId: 'b', passive: false, trickleIce: true,
        relayConfig: {urls: ['wss://x'], redundancy: 2, manualReconnection: false, warnOnRelayFailure: false},
        rtcConfig: {iceServers: [], iceTransportPolicy: 'all'}}, 'r'))
})

test('beforeunload leaves every room', () => {
    const {facade, module, listeners} = make()
    facade.joinRoom({appId: 'app'}, 'r1')
    facade.joinRoom({appId: 'app'}, 'r2')
    listeners.beforeunload()
    assert.equal(module.calls.filter(c => c.name === 'leave').length, 2)
})

test('observe sees each crossing, as copies; a throwing observer is detached', async () => {
    const {facade, module} = make()
    const seen = []
    facade.observe({call: (n, a) => seen.push(['call', n, a]), result: n => seen.push(['result', n]),
                    callback: (n, a) => seen.push(['callback', n, a])})
    facade.joinRoom({appId: 'app'}, 'r').makeAction('x')
    module.fire('message', module.lastRoom, 'x', 'p', 0, new TextEncoder().encode('hi'), '')
    assert.deepEqual(seen.slice(0, 2).map(s => s.slice(0, 2)), [['call', 'Runtime.join'], ['result', 'Runtime.join']])
    const message = seen.find(s => s[1] === 'events.message')
    // No view on the wasm memory: no ArrayBuffer larger than the payload anywhere in the arguments.
    const buffers = []
    const walk = v => { if (v instanceof ArrayBuffer) buffers.push(v); else if (ArrayBuffer.isView(v)) buffers.push(v.buffer); else if (v && typeof v === 'object') Object.values(v).forEach(walk) }
    walk(message[2])
    assert.ok(buffers.every(b => b.byteLength <= 2))
    assert.deepEqual(message[2].find(a => a && a.format !== undefined), {format: 0, length: 2, value: 'hi'})
    facade.observe({call: () => { throw new Error('x') }})
    assert.doesNotThrow(() => facade.joinRoom({appId: 'app'}, 'r3'))
})
```

  Add the cases that complete the Review Focus items 3 and 4 the same way:
  - a refused `join` (`{code, message}`) throws an `Error` with that `code`;
  - `onJoinError` receives `{error, appId, roomId, peerId}`, on a microtask, for a
    `roomError` of `wrong_password`;
  - `getPeers()` is `{p1: null, p2: null}`;
  - `ping` resolves with the settled value;
  - a JSON message that does not parse is dropped with a warning.

  Run `$B2 -a --build-dir=bin/b-em toolset=emscripten libs/trystero/test/js//facade`.
  Expected: FAIL, `facade.mjs` missing.
- [ ] **Step 4: Write `facade.mjs`.** The facade:
  - keeps `rooms` (a `Map` from `appId + '\u0000' + roomId` to `Room`), `handles` (a `Map` from
    handle to `Room`) and `pending` (a `Map` from ticket to `{resolve, reject}`);
  - checks every config member against the supported tables (Task 21), refusing any other,
    then calls `module` through `cross(name, f)`, which tells the observer copies;
  - turns every event into a copy at once (a payload's bytes with `slice()`, never the view),
    then `queue`s the user's handler, inside `try`, reporting a throw with
    `error('Trystero action handler error:', err)`;
  - decodes JSON with `JSON.parse`, inside `try`: a message whose JSON does not parse is
    dropped and warned, as a frame of another format is.

  Write `index.d.mts` from Trystero 0.26.0's `types.d.mts`, for the members kept, with a header
  comment naming the source; `LICENSE-TRYSTERO.txt` ships beside it. Write `package.json`:

```json
{
    "name": "@webcpp/trystero",
    "version": "0.1.0",
    "private": true,
    "description": "Trystero's API, backed by webcpp::trystero compiled to WebAssembly.",
    "license": "BSL-1.0 AND MIT",
    "type": "module",
    "exports": {".": {"types": "./index.d.mts", "default": "./index.mjs"}},
    "sideEffects": ["./index.mjs"],
    "files": ["index.mjs", "facade.mjs", "index.d.mts", "trystero.js", "trystero.wasm",
              "LICENSE_1_0.txt", "LICENSE-TRYSTERO.txt", "README.md"]
}
```

  Its licence names MIT for the derived types.
- [ ] **Step 5:** Run the facade's tests. Expected: PASS, every case.
- [ ] **Step 6: Write `bindings.cpp` and `js/Jamfile`,** as `example/chat/chat.cpp` binds
  through embind:
  - The module's one object is `Runtime`, which owns, in this order, the io_context,
    `browser::event_loop`, `browser::backend` and `js::runtime<val_events>`. There is no static
    state: the facade holds the one `Runtime`.
  - `val_events` turns each event into a call of the JavaScript `events` object: a
    `raw_payload`'s bytes go as a `Uint8Array` view, which the facade copies before the call
    returns; text crosses through `loop.text`.
- [ ] **Step 7:** Build the package and run the lane:

```bash
$B2 -a --build-dir=bin/b-em toolset=emscripten libs/trystero/test/js//js
```

  Expected: the facade's tests pass and the package's directory is printed, holding the files
  of `package.json`'s `files`. Record the size of `trystero.wasm` in the ledger.
- [ ] **Step 8: Write `js/README.md`:**
  - what the package is;
  - the one line a Trystero page changes;
  - what is supported and what throws (a link to trystero's differences);
  - where it runs: a browser's window, in a secure context;
  - how b2 builds it;
  - that its files will change (not its surface) when trystero moves onto webcpp::asio;
  - the licences.

  No `npm install` line until G6 (the site's page test enforces the same).
- [ ] **Step 9: Commit** trystero, then the superproject:
  - trystero: `"trystero: the JavaScript package, Trystero's API over the runtime, events on a microtask, every option implemented or refused"`;
  - superproject: `"webcpp.js-test: a library's JavaScript tested by node, a test of b2's that fails on no test; libs/trystero: the JavaScript package"`.

## Task 24: The package in the interop lane, Trystero's sketch among its scenarios

**Files:**
- Create (trystero):
  - `test/oracle/interop/api.html`: a page that imports the package through an import map and
    exposes the scenario's commands, as `js.html` does;
  - `test/oracle/interop/cursors.js`: Trystero's cursors sketch, its import naming the package
    (R14);
  - `test/oracle/interop/cursors_page.html`: the sketch's page in the test, with recording
    helpers.
- Modify (trystero): `test/oracle/interop/run.mjs` (`--api <package directory>`),
  `test/oracle/Jamfile` (`interop_api`), `README.md` (the derived parts: `cursors.js`, from
  trystero.dev, beside `js/index.d.mts`, M14).

**Interfaces:**
- Consumes: Task 23's package directory, the release bytes it ships (M12).
- Produces: the driven test `interop_api` (emscripten), in the lane `interop`.

- [ ] **Step 1: Write the failing scenarios** in `run.mjs`, under `--api`, each a `scenario`
  as the existing ones are, the package page against the peer named:
  - `api pair with Trystero JS`: join, then one message each way of each format (text, JSON,
    binary of 100,000 bytes with metadata). Each side records content, sender and metadata;
    `onPeerJoin`, and `onPeerLeave` after the other leaves.
  - `api pair with a password`; `api wrong password`: `onJoinError` on the package's side,
    with the Trystero JS peer's id.
  - `api two rooms of one app`: two rooms of one app on each side; both rooms meet, over one
    connection. The Trystero JS page counts its `RTCPeerConnection`s, as `js.html`'s
    `connections` list does, and sees one.
  - `api with the C++ peer`: the package meets `peer.cpp`, the browser's C++ peer.
  - `api ping`: a number of milliseconds.
  - `api leave and join again`: a new room, met again.
  - `api handler leaves its room`: a handler that calls `leave` from `onMessage`; the room
    leaves, the page logs no uncaught error, and the other room of the app goes on.
  - `cursors sketch`: two `cursors_page.html` pages run `cursors.js`, whose
    `renderPeerCursor`, `removePeerCursor` and `colorForPeer` record calls. After `onPeerJoin`
    is seen in both, the test moves the pointer in one page (`Input.dispatchMouseEvent`) every
    100 ms until the other draws it, for at most 15 s (M4), and checks the fractions sent.
    Closing one page calls `removePeerCursor` in the other.

  Add to `test/oracle/Jamfile`, the driver given the package's directory as its program (the
  drive's sources are the package target, so the release bytes are what runs):

```jam
webcpp.drive interop_api : /webcpp/trystero/js//package
  : <webcpp-drive-timeout>900
  : interop/run.mjs : --api : emscripten ;
webcpp.lane interop : interop_browser interop_native interop_api : native emscripten ;
```

  If `webcpp.drive` refuses a `make` target as its source, record it, and give it instead
  `../../js/bindings.cpp` with `<library>/webcpp/trystero/js//flags` (which carries
  `<variant>release`), so that the bytes are built with the package's own flags.
- [ ] **Step 2:** Run `$B2 -a --build-dir=bin/b-em toolset=emscripten libs/trystero/test/oracle//interop_api`.
  Expected: FAIL. `--api` is unknown to `run.mjs`.
- [ ] **Step 3: Implement `--api`.**
  - It serves the package's directory at `/package/`.
  - It starts Chrome with the flags the other modes use, the host resolver rule included.
  - It opens `api.html?relays=<local>` beside `js.html?relays=<local>`. The package page
    passes `relayConfig: {urls: [relay]}`, and `rtcConfig: {iceServers: []}` as the Trystero JS
    page has none.
  - The cursors pages cannot be given a relay without changing the sketch. `run.mjs` adds a
    script with `Page.addScriptToEvaluateOnNewDocument`, in the tests alone, that replaces
    `window.WebSocket` with a subclass connecting to the local relay whatever URL it is given,
    in every frame. Every redundant relay socket (5 by default) then reaches the one local
    relay, which delivers each event once per socket. Whether both sides drop the duplicates
    is the scenario's check, not a premise: each message is counted once on each side, and
    each `onPeerJoin` once per peer (M19).
- [ ] **Step 4:** Run. Expected: PASS, every scenario. Then run the whole `interop` lane
  (`interop_browser`, `interop_native`): no regression.
- [ ] **Step 5: Commit** trystero, then the superproject pointer:
  - trystero: `"trystero: the JavaScript package's release bytes meet Trystero JS 0.26.0 in the interop lane, Trystero's cursors sketch among its scenarios"`;
  - superproject: `"libs/trystero: the package in the interop lane"`.

## Task 25: trystero's page: the package, its differences and its measure

**Files:**
- Create (trystero): `doc/javascript.adoc`.
- Modify (trystero): `doc/trystero.adoc` (includes it), `doc/differences.adoc` (a section "The
  JavaScript package"), `doc/roadmap.adoc` (the three examples of trystero.dev as the measure:
  cursors runs; file transfer and video chat wait for their features), `doc/testing.adoc` (the
  `js` lane and `interop_api`), `README.md`.
- Modify (superproject): `tools/doc/doc.jam` (the attribute `webcpp-site`), `AGENTS.md`
  chapter 1 (a library's `js/`, its JavaScript face, O4).

**Interfaces:**
- Consumes: the specification of Task 21; the shared connection's pages (in `main` since Task
  20 Step 4).
- Produces: the AsciiDoc attribute `{webcpp-site}`: `../../` in the site's layout
  (`-sWEBCPP_INDEX=site`), where a library's page sits at `libs/<name>/`, and
  `https://webcpporg.github.io/` otherwise (M5).

- [ ] **Step 1: Write the failing test** of the attribute in `tools/test/doc_test.py`: a
  fixture page linking `{webcpp-site}javascript/` resolves to `../../javascript/` in the site's
  layout and to the preview's URL in the other. Run `python3 tools/test/doc_test.py`.
  Expected: FAIL. Then add the attribute to `doc.jam`; run again. Expected: PASS.
- [ ] **Step 2:** Write `doc/javascript.adoc` (R10), with no listing:
  - what the package is;
  - `b2 toolset=emscripten libs/trystero/js//package`, from the Jamfile's own words through a
    `tag::` region;
  - the surface kept, and the members refused;
  - the link `{webcpp-site}javascript/`, to the site's page "Using a webcpp library from
    JavaScript".
- [ ] **Step 3:** Update the differences (every item of Task 21's list), the roadmap, testing,
  the README, and AGENTS.md chapter 1.
- [ ] **Step 4:** Run trystero's doc and doc-check, in both layouts:
  `$B2 -a --build-dir=bin/b-doc libs/trystero/doc` and
  `$B2 -a --build-dir=bin/b-doc-site libs/trystero/doc -sWEBCPP_INDEX=site`. Expected: green,
  every count computed.
- [ ] **Step 5: Commit** trystero, then the superproject:
  - trystero: `"trystero: the JavaScript package on its page, its differences and the measure of trystero.dev's examples"`;
  - superproject: `"doc.jam: {webcpp-site}, relative in the site's layout; AGENTS.md: a library's js/; libs/trystero: its page names the package"`.

## Task 26: The site serves the package; the JavaScript path works alone

**Files:**
- Modify (superproject): `tools/site/site.jam` (the package into `bin/site.tmp/packages/trystero/`),
  `tools/site/site_test.py`.
- Modify (site):
  - `Jamfile` (`packages`, which places the package and `bindings.cpp` in
    `build/packages/trystero/` and writes `build/packages/required`);
  - `tools/dev.mjs` (the package fetched with the examples' wasm, or the reason it is not);
  - `tools/build.mjs` (an import map per surface page; esbuild's `external` for the package's
    name);
  - `package.json` (`typecheck` becomes `tsc --noEmit && node tools/typecheck.mjs`);
  - `tsconfig.package.json` (new: the package's types, by `paths`, and `test/types/`);
  - `tools/typecheck.mjs` (new), `tools/packages.mjs` (new);
  - `test/types/trystero_api.ts` (new: the API as Trystero's README uses it);
  - `README.md`, `test/readme_test.mjs`, `test/dev_test.mjs`;
  - `test/package_name_test.mjs` (new).

**Interfaces:**
- Consumes: `/webcpp/trystero/js//package` built for emscripten, as Task 13 recorded that `b2
  site` builds a target for emscripten (a property on the reference, or a second b2 with its
  own `--build-dir`); this task uses the mechanism Task 13's ledger entry names.
- Produces:
  - `bin/site/packages/trystero/` holds the package's files and
    `source/bindings.cpp`, the C++ the cursors page's C++ pane shows, which `dev` also
    fetches;
  - `build/packages/trystero/` in the site holds the same, for `npm test`, `typecheck` and
    `dev`;
  - `tools/packages.mjs`:
    - `packageDirectory(): string | null`, `build/packages/trystero/` when it holds a
      `package.json`, else null;
    - `packageName(): string`, read from it;
    - `packagesRequired(): boolean`, true when `build/packages/required` exists (b2 wrote it);
    - `needPackage(t): boolean`, for a `node:test` test: true when the package is there;
      when it is absent and not required, it calls `t.skip('the package is not in
      build/packages/trystero/: run npm run dev once the preview serves it, or b2 site')` and
      returns false; when required and absent, it throws, naming it. `t.skip()` does not end
      a test function, so every test that uses it starts with
      `if (!needPackage(t)) return` (m5).

- [ ] **Step 1: Write the failing tests.**
  - **`site_test.py`:** after `b2 site`, `bin/site/packages/trystero/` lists exactly the
    package's `files` and `source/bindings.cpp`, and every one is in `precache.json`.
  - **`test/dev_test.mjs`,** with the fake fetch of Task 5's test:
    - with no local build, `dev` downloads every file of `packages/trystero/` from the preview,
      as it downloads the wasm, and checks each against `precache.json`; a file whose hash
      differs fails, naming it;
    - when the preview's `precache.json` lists no `packages/trystero/` (before increment 4's
      publication), `dev` prints "the preview serves no packages/trystero/ yet: the cursors
      page will say that its package is missing; b2 site-serve builds it" and goes on serving
      the rest.
  - **`test/package_name_test.mjs`** (each case through `needPackage`):
    - every `import ... from '<x>'` of `examples/**` whose `<x>` is not relative names
      `packageName()`;
    - no page or README of the site shows `npm install <name>` while the package's
      `package.json` says `"private": true` (G6).
  - **`tools/typecheck.mjs`:** runs `tsc --noEmit -p tsconfig.package.json` when
    `packageDirectory()` is set, else prints the skip reason and exits 0, unless
    `packagesRequired()`, when it exits 1 naming the package.
  - **`readme_test.mjs`:** the JavaScript path 1 says that `npm run dev` fetches the package
    too, and what `npm test` skips without it.
- [ ] **Step 2:** Run `npm test` in the site and `python3 tools/site/site_test.py`. Expected:
  FAIL.
- [ ] **Step 3: Implement.**
  - `site.jam`: depend on the package and copy its directory, with
    `libs/trystero/js/bindings.cpp` as `source/bindings.cpp`, before the link check.
  - The site's `Jamfile`: the `packages` target copies the same into `build/packages/` and
    writes `build/packages/required`. It is a dependency of `test`; the `site-test` lane,
    which builds no emscripten, copies `index.mjs`, `facade.mjs`, `index.d.mts` and
    `package.json` straight from `libs/trystero/js/` into `build/packages/trystero/`, and
    `bindings.cpp` into `build/packages/trystero/source/bindings.cpp`, where Task 28's test
    reads it, without the wasm. A test that needs the wasm runs in the browser lane only.
  - `dev.mjs`, `typecheck.mjs`, `tsconfig.package.json`, `test/types/trystero_api.ts` and the
    README.
- [ ] **Step 4:** Run, in the site alone (`build/packages/` removed): `npm test` and
  `npm run typecheck`. Expected: PASS, with the package's tests and type check reported as
  skipped, each with its reason. Then `$B2 -a /webcpp/site//test`. Expected: PASS, none
  skipped.
- [ ] **Step 5: Commit** both:
  - superproject: `"b2 site: trystero's JavaScript package at packages/trystero/, with the C++ its page shows"`;
  - site: `"The package served at packages/trystero/, fetched by dev, its name read from its manifest; the JavaScript path works without it"`.

## Task 27: Sizes, measured by the build: trystero

**Files:**
- Create (site):
  - `tools/sizes.mjs`;
  - `src/components/size_table.tsx`;
  - `test/sizes_test.mjs`;
  - `test/fixtures/sizes.json` (the fixture `dev` and the page tests render with).
- Modify (site):
  - `package.json` and `package-lock.json` (`trystero` 0.26.0, a dev dependency);
  - `test/package_test.mjs` (its pin);
  - `Jamfile` (`sizes`, which measures into the build directory, and `sizes-check`);
  - `src/context.ts` (`SiteContext.sizes`);
  - `src/components/library_table.tsx` (a column);
  - `pages/javascript.oui` (a section "Size, measured", with `SizeTable()`);
  - `tools/dev.mjs` (the fixture's sizes).
- Modify (superproject):
  - `tools/site/site.jam` (`sizes` before the context, `sizes.json` into `bin/site/`,
    `sizes-check` after the assembly);
  - `tools/ci/assemble.py` and `assemble_test.py` (`--sizes`: one line under each measured
    library's page title).

**Interfaces:**
- Consumes: O5 resolved; the plan is written for its recommendation (R16).
- Produces:

```
node tools/sizes.mjs --measure trystero=<package dir>:trystero:joinRoom,selfId
                     [--measure xstate=<dir>:xstate:createMachine,createActor] --out <sizes.json>
node tools/sizes.mjs --check <site dir>
```

  - Each `--measure` is `<library>=<port's directory>:<original's npm package>:<exports>`. The
    site's `Jamfile` writes these words. The port's side is every `.js`, `.mjs` and `.wasm` file
    of the directory, which is what a page loads.
  - `--out` is in the build directory; `b2 site` copies it to `bin/site/sizes.json`. It is
    never committed (R16).

```json
{
    "trystero": {
        "original": {
            "name": "Trystero",
            "version": "0.26.0",
            "exports": ["joinRoom", "selfId"],
            "raw": 0,
            "gzip": 0,
            "brotli": 0
        },
        "port": {
            "contains": "the package: its facade, its glue and its wasm",
            "files": ["facade.mjs", "index.mjs", "trystero.js", "trystero.wasm"],
            "raw": 0,
            "gzip": 0,
            "brotli": 0
        }
    }
}
```

  - **The original:** esbuild 0.28.2 bundles `export {<exports>} from '<original>'`, minified,
    `format: 'esm'`, `platform: 'browser'`. A size is measured raw, gzip at level 9 and brotli
    at quality 11, with node's `zlib`. Every number is a count of bytes, never typed.
  - **`SiteContext.sizes`:** `Record<string, Sizes>`, the measurement's content.
  - **`SizeTable()`:** the measured libraries with the three sizes of each side, and what the
    port's side contains. Each number is rendered as `<data value="<bytes>">` with its unit
    computed (`12.3 KB` from 12,345 bytes, 1 KB = 1000 bytes); so is the library list's column.
  - **`--check <site dir>`:** every `<data value>` of a size in the site's pages, and every
    line `assemble.py --sizes` wrote under a library's title, equals the measurement in
    `sizes.json`; a difference fails, naming the page, the library, the side and the field.

- [ ] **Step 1: Write the failing tests** `test/sizes_test.mjs`:
  - over a fixture port directory and a fixture original (a local module):
    - the three sizes of each side are the byte counts of the files, of their gzip and of
      their brotli;
    - a `.d.mts`, a README or a licence in the directory is not counted;
  - a `--measure` whose directory holds no `.js`, `.mjs` or `.wasm` fails, naming it;
  - an export the original does not export fails, naming it, as one the port's `index.mjs`
    does not export does (read with esbuild's metafile), so that the words of the Jamfile
    stay true;
  - `SizeTable` renders one row per library from a fixture measurement, each size a `<data>`
    with its bytes, never a number from the page source;
  - `--check` passes on a fixture site rendered from the same measurement, and fails, naming
    the page, when one `<data value>` differs.
- [ ] **Step 2:** Run `npm test`. Expected: FAIL.
- [ ] **Step 3: Implement** `sizes.mjs`, `SizeTable`, the column ("Size, brotli: webcpp /
  original") and the context. `dev` renders with `test/fixtures/sizes.json`, which the page
  says are a fixture's in dev only (its band names `dev`).
- [ ] **Step 4: Wire it into `b2 site`.** The site's `Jamfile` `sizes` runs
  `npm run build -- --sizes ...` through `site.npm`, after `/webcpp/trystero/js//package`,
  into the build directory; `site.jam` writes the measurement into the context before the
  pages render, copies it to `bin/site/sizes.json`, and runs `sizes-check` after the
  assembly. `assemble.py --sizes`: its test first (a library page with an entry gains, right
  after its `<h1>`, one `<p class="webcpp-size">` with `<data>` values; a page without one is
  unchanged), then the code.
- [ ] **Step 5:** Run `$B2 -a --build-dir=bin/b-site site`. Expected: green, `sizes-check`
  among its steps. The libraries page, trystero's page and the JavaScript door show the sizes.
  Look at the three pages at 360 and 1280 px, light and dark; fix what looks off.
- [ ] **Step 6: Commit** both:
  - site: `"Sizes, measured by every build: trystero's package beside Trystero 0.26.0, raw, gzip and brotli, the pages held to the measurement"`;
  - superproject: `"b2 site measures the sizes into bin/site and checks the pages against them; assemble.py writes each measured library's size under its title"`.

## Task 28: The collaborative cursors

**Files:**
- Modify (outside the repositories, the evidence): `~/.buzz/RESEARCH/TRYSTERO_DEV_EXAMPLES/`,
  which already holds the fetched page as `cursors.html` (SHA-256 `3d554609...`, checked by the
  review): its `README.md` gains the URL, the date, the HTML's SHA-256, the extraction rule and
  the text's SHA-256 (`da3d8f82...`), each in full.
- Create (site):
  - `examples/cursors/cursors.js`: Trystero's sketch, its import the package's (R1);
  - `examples/cursors/LICENSE-TRYSTERO.txt`: a copy of `libs/trystero/LICENSE-TRYSTERO.txt`;
  - `examples/cursors/README.md`: the source and what is the site's (Step 1);
  - `examples/cursors/draw.js`: `renderPeerCursor`, `removePeerCursor`, `colorForPeer`,
    defined on `globalThis`;
  - `examples/cursors/surface.ts`: the frame's own script (Join, the notice, `observe`, the
    trace to the parent page);
  - `src/components/frame.tsx`, `src/components/example_surface.tsx`, `src/scopes.ts`;
  - `pages/examples/cursors.oui`, `pages/examples/cursors/explained.oui`,
    `pages/examples/cursors/surface.oui`;
  - tests: `test/examples_provenance_test.mjs`, `test/draw_test.mjs`, `test/scopes_test.mjs`,
    `test/surface_test.mjs`.
- Modify (site):
  - `src/library.ts` (the new components);
  - `src/server.tsx`, `src/head.ts` (a `Frame` root: noindex, no card, no sitemap entry, the
    import map);
  - `tools/indexing.mjs`, `tools/cards.mjs` (frames left out);
  - `src/components/visualizer.tsx` (the optional fifth prop `frame`, and a bounded trace);
  - `pages/examples.oui` (the gallery), `pages/javascript.oui` ("Using a webcpp library from
    JavaScript");
  - `seo/queries.json` ("C++ WebRTC", "C++ peer to peer browser", "use C++ from JavaScript"
    answered by the cursors page or the JavaScript door).

**Interfaces:**
- Consumes:
  - the package at `packages/trystero/`, its `source/bindings.cpp` and its `observe`
    (Tasks 23, 26);
  - `TraceEvent` (Task 15);
  - `Visualizer` and `frame` (Task 16).
- Produces:
  - **`Frame(title, body)`:** the root of a surface page. It renders:
    - a `<main>` without header, band or footer;
    - `<meta name="robots" content="noindex">`;
    - when the page declares imports, `<script type="importmap">` with them, each relative to
      the root;
    - no social card, no sitemap or `llms.txt` line.
  - **`ExampleSurface(src, title, peers)`:** `peers` frames of `src`, side by side from 768
    px and stacked below. Each is `<iframe title=... loading="lazy">` with a reserved height,
    so that layout shift stays under 0.1. A Join button and G7's notice sit above them.
  - **`Visualizer(example, js_source, cpp_source, options, frame)`:** with `frame` set, the
    visualizer loads no wasm. It plays the `TraceEvent`s the named surface posts with
    `postMessage({type: 'webcpp-trace', event})`, from the same origin only. It keeps the last
    256 events, and replays the window it holds when the reader presses Play. `options.anchors`
    maps a crossing's name to a text of the JavaScript source (below).
  - **`src/scopes.ts`:**

```ts
export type Region = {name: string; first: number; last: number}   // 1-based lines, tags excluded
export function regionsOf(source: string): Region[]                // every tag::<name>[] ... end::<name>[]
export function scopesFrom(regions: Region[], file: string): (e: TraceEvent) => TraceEvent[]
// js_to_wasm X  -> [js_to_wasm X, cpp_enter X (file, regions[X].first)]
// return X      -> [cpp_exit X, return X]
// wasm_to_js X  -> [cpp_enter X, wasm_to_js X, cpp_exit X], for an events.* callback
// a name with no region -> the event alone, and the build fails if the page names it
export function anchorsOf(source: string, anchors: Record<string, string>): Record<string, number>
// each crossing's name -> the 1-based line of the JavaScript source that holds its text,
// found exactly once, else the build fails naming the text (M11). For the cursors:
// Runtime.join -> "joinRoom(", Runtime.makeAction -> "room.makeAction(",
// Runtime.send -> "cursor.send(", events.message -> "cursor.onMessage =",
// events.peerLeave -> "room.onPeerLeave ="
```

  - **`draw.js`** (globals, for the unchanged sketch):

```js
globalThis.renderPeerCursor = (peerId, x, y, color) => { /* clamp, validate, at most 64 */ }
globalThis.removePeerCursor = peerId => { /* remove its element */ }
globalThis.colorForPeer = peerId => '#rrggbb'   // a stable colour from the id, by its hash
```

- [ ] **Step 1: Archive the source and write the provenance (I7, O3).**
  - The archive is `~/.buzz/RESEARCH/TRYSTERO_DEV_EXAMPLES/cursors.html`. Check its SHA-256
    is `3d554609e6aedda46d691fdc8475f1bdcbcb86858af10312745e15ed41ecbad6`, and add to the
    archive's `README.md` the URL (https://trystero.dev/examples/collaborative-cursors/), the
    date of the fetch (2026-10-11), that hash, the extraction rule below and the text's
    SHA-256. The archive stays out of the site's repository: it carries trystero.dev's own
    markup.
  - **The extraction rule:** the page's one `<pre>` element; its text with every tag removed
    and every HTML entity decoded (its `textContent`); `\r\n` turned into `\n`; trailing line
    ends trimmed and one `\n` added. On the archived page it gives 24 lines whose SHA-256 is
    `da3d8f82a3b587106406fc783bbf3a2e334c713ec8defb10ef0d122d99480133`.
  - Write `examples/cursors/cursors.js`: the extracted text, its first line replaced by
    `import {joinRoom, selfId} from '@webcpp/trystero'`.
  - Write `examples/cursors/README.md`:
    - the URL, the date of the fetch, the HTML's SHA-256, the archive's path, the extraction
      rule and the text's SHA-256;
    - the one line changed;
    - that the page states no licence, that trystero.dev's home names MIT for Trystero, and
      that the copy relies on the owner's ruling of 2026-10-10, "Trystero's licence": every
      part of Trystero is MIT, the examples of trystero.dev included; `LICENSE-TRYSTERO.txt`
      beside it;
    - what is the site's: `draw.js`, `surface.ts`, the pages.
- [ ] **Step 2: Write the failing tests.**
  - **`test/examples_provenance_test.mjs`:**
    - `examples/cursors/cursors.js` with its first line replaced by
      `import {joinRoom, selfId} from 'trystero'` hashes to the text's SHA-256 the README
      records;
    - its first line is `import {joinRoom, selfId} from '<packageName()>'`;
    - the README records the HTML's SHA-256 and the extraction rule, and the text's hash
      is also a constant of the test itself (`da3d8f82...`, in full), so that rewriting the
      README's hash alone does not pass; the archive's extraction, run once in Step 1, is what
      gave it;
    - when `../libs/trystero/test/oracle/interop/cursors.js` exists, it is the same bytes
      (R14).
  - **`test/draw_test.mjs`,** over a minimal DOM (`node:test` with a fake `document` holding
    `createElement`, `append`, `remove` and `style`), the Review Focus 1 cases:
    - `renderPeerCursor('p', 0.5, 0.25, '#ff0000')` places the element at 50% and 25%;
    - `x` of `-3`, `7`, `NaN`, `undefined` and `'1'` are clamped or refused: a non-number is
      not drawn;
    - a colour `'red; background:url(x)'`, `'<b>'` or a string of a megabyte is replaced by
      `colorForPeer('p')`;
    - the 65th distinct peer is not drawn;
    - `innerHTML` is never used (a static check: the file has no `innerHTML`);
    - `removePeerCursor` of an unknown id does nothing;
    - `colorForPeer` is stable for an id and a valid `#rrggbb`.
  - **`test/scopes_test.mjs`:**
    - `regionsOf` over a fixture with two tags;
    - `scopesFrom` maps each kind as the interface says;
    - `anchorsOf` finds each anchor of the cursors in `cursors.js` once, and fails on a text
      found twice or never;
    - through `needPackage`, over `build/packages/trystero/source/bindings.cpp`, every name the
      cursors page lists has a region.
  - **`test/surface_test.mjs`:**
    - the surface page renders with `noindex`, an import map naming the package at
      `../../../packages/trystero/index.mjs` (relative from `examples/cursors/surface/`), and
      no header;
    - it is absent from `sitemap.xml` and `llms.txt`;
    - the cursors page renders two frames with a reserved height, G7's notice, a Join
      `<button>` with an `aria-label`, and the visualizer whose JavaScript pane holds
      `cursors.js` and whose C++ pane holds `bindings.cpp`;
    - the surface's CSS sets `touch-action: none`;
    - a message posted from another origin is ignored by the visualizer.
- [ ] **Step 3:** Run `npm test`. Expected: FAIL.
- [ ] **Step 4: Write the components, the surface and the pages.**
  - **`surface.ts`,** bundled by esbuild with the package's name `external`:
    - waits for the parent's `{type: 'webcpp-join'}` or its own Join;
    - imports the package and calls `observe`, turning each crossing into a `TraceEvent`
      posted to the parent (the observer's arguments are copies, so the clone is small);
    - imports `draw.js`, then `cursors.js` (both copied as they are into
      `assets/examples/cursors/`, so that a reader sees Trystero's file as it ships).
    - If the import fails (the wasm blocked) or `joinRoom` throws (an insecure context, no
      WebRTC), the frame shows the error's message in text and posts
      `{type: 'webcpp-error', message}`. The visualizer then says the example could not
      start, and why.
  - **The live page:** the two surfaces ("you" and "a second peer, in this page"), the
    visualizer fed by the first, and a link to open the surface alone in another tab or
    device.
  - **The explanation:**
    - the one line changed, shown from the file itself;
    - what the package does under it;
    - the helpers the site wrote and why;
    - what crosses the bridge per pointer move (the sketch sends on every move, unthrottled,
      as written);
    - how to build the package, from the Jamfile's `tag::` region;
    - the licences and the source (the README's facts);
    - that the example meets any Trystero JS page running the same sketch (G7).
  - **The gallery and the JavaScript door:** the cursors move from "coming" to the list.
    "Using a webcpp library from JavaScript" shows the import map and the import line of the
    example, read from the files. The roadmap lines delivered are removed.
- [ ] **Step 5:** Run `npm test` and `npm run typecheck`. Expected: PASS.
- [ ] **Step 6:** Run `$B2 -a --build-dir=bin/b-site site-serve`. Open the cursors page in
  Chrome on 127.0.0.1.
  - The page makes no request beyond 127.0.0.1 before Join (DevTools, Network).
  - At 360 px with touch emulation, a finger in one surface draws in the other and the page
    does not scroll (with the local relay of Task 29's driver, or after G7 over the public
    relays).
  - After G7 only: with the network open, the two surfaces meet over Trystero's public relays,
    each drawing the other's cursor. This is the only run of this task that reaches a public
    relay.
  - Look at 360, 768, 1280 and 1440 px, light and dark, with reduced motion and with sound;
    fix every fault before the commit.
- [ ] **Step 7: Commit** (site): `"The collaborative cursors: Trystero's own sketch, its import
  changed, on webcpp's trystero, in the visualizer"`.

## Task 29: Increment 4 in headless Chrome, its review and its publication

**Files:**
- Modify (site): `test/browser/run.mjs`, `test/browser/scenarios.mjs`, `tools/serve.mjs` (the
  `.wasm` type, if Task 5 left it out).
- Modify (site): `Jamfile` (`browser-test` depends on `/webcpp/trystero/test/oracle//node-modules`,
  which brings `relay.mjs`'s `ws`, M6).

- [ ] **Step 1: Make the network guard a step (M16).** `run.mjs` starts Chrome with
  `--host-resolver-rules=MAP * ~NOTFOUND, EXCLUDE 127.0.0.1` beside Task 18's flags. A canary
  scenario runs first: a page's `fetch('https://example.com/')` and a `new
  WebSocket('wss://relay.damus.io')` both fail to resolve; if either succeeds, the run stops
  before any other scenario.
- [ ] **Step 2: Write the failing scenarios.**
  - **Two pages, over the local relay.**
    - `run.mjs` starts `libs/trystero/test/oracle/interop/relay.mjs` on 127.0.0.1, its path
      given by b2.
    - It redirects the surfaces' WebSockets to it as Task 24 does.
    - It opens the cursors page twice and presses Join in both.
    - After `onPeerJoin` is seen in both (the visualizer's log), it moves the pointer in the
      first page's surface every 100 ms until the second page's surfaces draw it, for at most
      15 s (M4), at the fractions sent.
    - Each visualizer's log pane holds, as a subsequence, `Runtime.join`, a `Runtime.send`
      crossing, then an `events.message` callback.
  - **The in-page pair:** one page alone; its two surfaces meet each other, within the same
    bound.
  - **Without the wasm:** with `Network.setBlockedURLs` on `*/packages/trystero/*.wasm`, the
    surface shows the failure in text, the visualizer says it could not start, and the console
    holds no uncaught error (Review Focus 2).
  - **Without WebRTC** (M17): with `delete window.RTCPeerConnection` added by
    `Page.addScriptToEvaluateOnNewDocument`, Join shows "needs WebRTC" in text, and the console
    holds no uncaught error.
  - **Nothing before Join:** no WebSocket opens before the button is pressed
    (`Network.webSocketCreated` counted).
  - **Offline:** after the PWA's install, the cursors page and its package load offline, and
    Join fails in text (no relay), without an uncaught error.
  - **The wasm's type** (M20): every `.wasm` response, from the server and from the service
    worker's cache (offline), has `Content-Type: application/wasm`, and the console holds no
    Emscripten warning about streaming compilation.
  - **At 360 px with touch:** `Input.dispatchTouchEvent` moves the cursor and
    `window.scrollY` stays 0.
- [ ] **Step 3:** Run with the site built: `$B2 -a /webcpp/site//browser-test`. Expected:
  FAIL on the new scenarios.
- [ ] **Step 4:** Implement what the scenarios need in `run.mjs`, `serve.mjs` and the service
  worker (a cached `.wasm` keeps its type).
- [ ] **Step 5:** Run. Expected: PASS, every scenario, old and new.
- [ ] **Step 6: Run the increment's verdicts,** each from scratch, and record each in the
  ledger:
  - trystero's lanes, native and emscripten, with and without exceptions;
  - its `interop` and `js` lanes;
  - its doc (both layouts) and lint;
  - `$B2 -a --build-dir=bin/b-site site`, the site's `npm test` and `typecheck` (in the site
    alone, then under b2), the browser lane;
  - every `tools/**/*_test.py` as `python3 <path>`.
- [ ] **Step 7:** The controller dispatches the review of increment 4 (trystero, the site and
  the superproject), on the most capable model, with this plan's Review Focus. Fix each
  finding, with a failing test first.
- [ ] **Step 8 (controller, G2; G7 a precondition):** G7 is resolved before this push, which
  publishes the cursors page: the site carries the option the owner chose (G7's list says what
  (b) and (c) change in Task 28), never (a) by default. With the owner's agreement, push
  trystero, the site, then the superproject. Watch the CI publish the preview, and open the
  cursors page on https://webcpporg.github.io/ in two browsers (under (a) or (b), joining; under
  (c), the recorded trace).

---

# Increment 5: the state machine, on the server, and C++ calls the browser

## Task 30: The state machine example, and xstate's size

**Files:**
- Create (site):
  - `examples/state_machine/Jamfile`, `examples/state_machine/state_machine.ts`;
  - `src/components/machine_view.tsx`: the machine as tiles, from its JSON;
  - `pages/examples/state-machine.oui`, `pages/examples/state-machine/explained.oui`;
  - `test/state_machine_test.mjs`.
- Modify (site):
  - `app/site_app.cpp`: trace scopes in `machine_host::create`, `send` and the named
    implementations, which compile to nothing in the site's own wasm;
  - `pages/examples.oui`, `pages/javascript.oui`, `seo/queries.json` ("C++ state machine
    XState");
  - Step 8 only: `Jamfile` (`sizes` with `--measure xstate=...`).

**Interfaces:**
- Consumes: `machine_host`, `app/bindings.cpp`, `app//site-app`, `loadSiteApp`, `useMachine`
  (Task 13); `machines/player.json`, `machines/install.json` (Tasks 13, 14); `traced`,
  `WEBCPP_SITE_TRACE_SCOPE` (Task 15); `Visualizer` (Task 16); and, for Step 8 alone,
  `tools/sizes.mjs` and the `sizes` target (Task 27).
- Produces:
  - **The program `state-machine`** (`examples/state_machine/Jamfile`), the module
    `StateMachine`, built from `../../app/site_app.cpp` and `../../app/bindings.cpp` with
    `<define>WEBCPP_SITE_TRACE` and `<include>../../include` (`webcpp_site/trace.hpp`), and with
    Task 13's link flags but `-sEXPORT_NAME=StateMachine`. It is the same API as `SiteApp`'s
    `MachineHost`.
  - **`MachineView(machine)`:** one tile per state of the named machine, the current one lit,
    and one `<button>` per event the machine names.
  - **The page's options:** `player` (the player's own machine, spec section 6: "one page shows
    the player's own machine") and `install`.

Task 30 waits for Task 27's tool for its last step only (I3): Steps 1 to 7 ship the example,
and Step 8 measures xstate once Task 27 is in the site's `main`.

- [ ] **Step 1: Write the failing tests** `test/state_machine_test.mjs`:
  - **On the built `build/examples/state-machine.mjs`:**
    - each option's machine plays a scripted sequence (player: `PLAY`, `TICK`, `PAUSE`,
      `SPEED` 2, `PLAY`, `END`, `RESTART`; install: `MISSING`, `ACCEPT`, `PROGRESS`,
      `INSTALLED`, `NEW_VERSION`, `UPDATED`);
    - it agrees with XState 5.33.2 at each step, value and context. XState is the oracle in
      the test alone (R11).
  - **The trace of `send('{"type":"PLAY"}')` under `traced`** holds, as a subsequence,
    `js_to_wasm send`, the C++ scope `send` with the event as argument, the scope of the
    implementation that assigns `played`, and `return send` with the snapshot.
  - **An event the state does not take** leaves the snapshot's value unchanged, as XState
    does.
  - **`MachineView`'s render:**
    - one tile per state of `player.json`, read from the JSON;
    - the lit tile is the snapshot's value;
    - every event is a `<button>` with an `aria-label`.
  - **The page's render:**
    - the machine's JSON in the JavaScript pane;
    - the C++ pane holds `app/site_app.cpp`;
    - the panel "The state React holds" shows the snapshot as the page's state.
- [ ] **Step 2:** Run `npm test` after `$B2 -a --build-dir=bin/b-site site`. Expected: FAIL.
- [ ] **Step 3: Implement** the program, the component, the script and the pages:
  - the explanation shows the same JSON run here and in the site's own player;
  - the gallery entry;
  - the JavaScript door's line on calling C++ from JavaScript.
- [ ] **Step 4:** Run. Expected: PASS.
- [ ] **Step 5:** Add the browser scenario to `test/browser/scenarios.mjs`: the page plays
  the player machine's sequence by its buttons, and the log pane ends with `done`. Run the
  browser lane. Expected: PASS.
- [ ] **Step 6:** `$B2 -a --build-dir=bin/b-site site-serve`. Look at the page at the four
  widths, light and dark, keyboard only; fix every fault.
- [ ] **Step 7: Commit** (site): `"The state machine: the site's own machines on webcpp::xstate
  in the visualizer, XState as the test's oracle"`.
- [ ] **Step 8: xstate's size, on the build that ships (I11), after Task 27.**
  - Add to the site's `Jamfile` `sizes`:
    `--measure xstate=<the directory of app//site-app>:xstate:createMachine,createActor`. The
    port is the site's own wasm as it ships: `machine_host`, its embind bindings,
    webcpp::xstate and Boost.JSON, untraced. Task 13 does not fix `site-app`'s variant, so the
    measurement names it: the `sizes` target builds `app//site-app` with `<variant>release` for
    the measurement, the variant `b2 site` ships (if Task 13's ledger records another variant
    shipped, the site's `pages` target gains `<variant>release` on its reference in the same
    commit, so that what is measured is what ships). Its `contains` says so, and the size table
    shows it.
  - Extend `test/sizes_test.mjs`: a measurement's `contains` is shown beside its port's sizes.
  - Run `npm test`, then `$B2 -a --build-dir=bin/b-site site`. Expected: green; the libraries
    page shows xstate's sizes beside XState 5.33.2's.
  - Commit (site): `"Sizes, measured: xstate's, the site's own wasm as it ships, beside XState
    5.33.2"`.

## Task 31: The site's own lanes, and a native program served as a component is (superproject)

**Amended after the final re-review (controller, 2026-10-11):**
- m11: `rule declared` returns only the program pairs recorded under the directories it is
  given: record each pair with its directory and filter in `declared`, so a run that loads a
  site example's Jamfile first (`b2 declared-lanes declared-targets` together, or `b2 site`
  when `/webcpp/site//pages` loads the examples before `write-declared`) leaks no `site`
  pair into `declared-targets` or `libraries.json`. The fixture test adds a combined
  `declared-lanes declared-targets` run asserting `declared-targets` prints no `site` line.
- m12: with the stub `site/Jamfile` present, `matrix.py plan` already plans the existing
  `site-test` lane; the assertion "no ordinary lane of `site`" allows `site-test` and looks
  only for lanes whose projects name `site/`.

**Files:**
- Modify (superproject):
  - `tools/webcpp.jam`:
    - `rule lane` accepts a Jamfile under `site/examples`, as the library `site`;
    - `library-of` returns `site` for a Jamfile under `site/examples`, so that a lane's line
      and a program's record name `site`, never `site/examples/<dir>`;
    - `webcpp.serve-native`;
  - `Jamroot`: a rule of its own, `lane-directories`, which returns `library-directories` and,
    when `site/` is checked out, every directory of `site/examples` that holds a Jamfile;
    `declared-lanes` reads `lane-directories`, while `declared-targets` keeps reading
    `library-directories` alone (N1). So the site's programs never reach `b2 declared-targets`,
    from which `matrix.py`'s `plan()` makes the ordinary lanes (`libs/<name>/test` and
    `libs/<name>/example`): the site's programs run only in its own lanes;
  - `tools/lint/compile_commands.py` and its test: `LANE` reads `site/examples/...` too;
  - `tools/ci/matrix.py`, `matrix_test.py`: `parsed_own_lanes` accepts
    `site/examples(/...)` for the library `site`;
  - `.github/workflows/library.yml`: the `own-lanes` job runs a lane of `site` from the
    superproject (its checkout is already recursive) with the setup of its target: Boost for
    native, and wasi-sdk, wasmtime, wit-bindgen and the WIT for wasip2 and wasip3;
    `tools/ci/actions_test.py` with it;
  - `tools/report/report.py` and its test, only if a `site` own lane's XML is not read as a
    library's;
  - `tools/component/serve.py`, `serve_test.py`: `--native PROGRAM`;
  - `tools/component/component.jam`: the native serve, and `rule serve-words ( version )`,
    factored from what it passes `serve.py`;
  - `tools/test/fixtures/site/examples/demo/Jamfile` and `demo.cpp` (new): a stand-in for the
    site's own lane, and the program it runs (a `main` returning 0);
  - `tools/test/webcpp_jam_test.py`: the lane tests, through `harness.py`, where the tree's
    lane tests live;
  - `AGENTS.md` chapter 9.

**Interfaces:**
- Consumes: asio's Tasks 2 and 3 in `main` (precondition: the same files).
- Produces:
  - **The site's own lanes (C1, N1):** a Jamfile under `site/examples/<dir>` may declare
    `webcpp.lane`; `b2 declared-lanes` prints it as `site <lane> site/examples/<dir> <target>
    <kind>`, and the CI runs it as a library's own lane. `b2 declared-targets` prints no line of
    the site.
  - **`serve.py --native PROGRAM`,** in place of `--wasmtime`, `--flags` and `--component`.
    It runs `PROGRAM --addr 127.0.0.1:0`, which must print
    `Serving HTTP on http://127.0.0.1:<port>/` on its standard error, the line wasmtime prints
    (`SERVING`). Then it is as for a component: the same requests, transcript, bounds and
    stopping.
  - **`webcpp.serve-native source : requirements * : stem : targets *`,** the test
    `<stem>_native` of b2's. It builds `source` natively and passes when its transcript of
    `<stem>.requests` equals `<stem>.expected`. The stem is given, so that a component and a
    native host share one pair.
  - **`component.serve-words ( version )`:** the words `serve.py` runs wasmtime with
    (`serve <flags>`), for the sessions of Task 32.

- [ ] **Step 1: Write the failing tests,** one per place the library of a lane is computed:
  - **the `lane` rule, `library-of`, `lane-directories` and `declared-targets`**, in
    `tools/test/webcpp_jam_test.py`, beside its lane tests, through `harness.py` over a scratch
    copy of the superproject. The fixture `tools/test/fixtures/site/examples/demo/` (its
    `Jamfile`: `webcpp.targets native wasip2 ; webcpp.run demo : demo.cpp ; webcpp.lane http :
    demo : native wasip2 ;`, and its `demo.cpp`) is copied to `site/examples/demo/`, with an
    empty `site/Jamfile` so that `site/` counts as checked out:
    - `b2 -d0 declared-lanes` prints `site http site/examples/demo native programs` and
      `site http site/examples/demo wasip2 programs`;
    - `b2 -d0 declared-targets` prints no line that starts with `site` or names
      `site/examples` (N1), and the same lines as without the fixture;
    - `python3 tools/ci/matrix.py plan` over that copy plans no ordinary lane of `site`, and
      the two own lanes;
    - the refusal: the same fixture copied to `site/other/demo/` and built directly,
      `b2 site/other/demo//http`, which loads the Jamfile, fails with the lane rule's message
      ("which is not a test or an example Jamfile of a library"); `declared-lanes` alone would
      never load it, so it would see nothing;
  - **`compile_commands.py`:** a `declared-lanes` line of `site/examples/demo` is read, and
    its programs reach the compile database;
  - **`matrix.py`:** those lines yield two own lanes for `site`; a directory outside
    `site/examples` is refused;
  - **`library.yml`** (`actions_test.py`): a `site` own lane on wasip2 gets the wasip2 setup;
  - **`serve.py`:**
    - a fixture native program (a Python script standing in, printing the line and answering
      HTTP with `http.server`) gives the transcript a component gives;
    - one that never prints the line fails with status 2 and its standard error;
    - it is stopped in every outcome, as the component cases check.
- [ ] **Step 2:** Run each, as `python3 <path>`: `tools/test/webcpp_jam_test.py`,
  `tools/test/jamroot_test.py`, `tools/lint/compile_commands_test.py`,
  `tools/ci/matrix_test.py`, `tools/ci/actions_test.py`, `tools/component/serve_test.py`.
  Expected: FAIL.
- [ ] **Step 3: Implement.** Then run them all, and every `tools/**/*_test.py`. Expected:
  PASS.
- [ ] **Step 4: Commit:** `"The site's own lanes, from site/examples; webcpp.serve-native: a
  native program served and compared with a component's transcript"`.

## Task 32: On the server

**Files:**
- Create (site), in `examples/on_the_server/`:
  - `answer.hpp`: `answer(method, target) -> webcpp::wasi::http::response`;
  - `calculator_http.cpp`, the component: `WEBCPP_WASI_HTTP_MAIN_BEGIN(method, target) return
    answer(method, target); WEBCPP_WASI_HTTP_MAIN_END()`. It is named after its transcript,
    since `webcpp.serve` takes the stem from the source;
  - `native_host.cpp`: Boost.Beast, synchronous, one connection at a time;
  - `calculator_http.requests`, `calculator_http.expected`;
  - `answer_test.cpp`;
  - `Jamfile`;
  - `boost_url.cpp`, only under R9's second branch.
- Create (site):
  - `tools/sessions.mjs`, `src/components/terminal.tsx`;
  - `pages/examples/on-the-server.oui`, `pages/examples/on-the-server/explained.oui`;
  - `test/sessions_test.mjs`, `test/terminal_test.mjs`.
- Modify (site):
  - `examples/calculator/`: its evaluation, factored into `evaluate.hpp` if Task 17 left it
    inside `calculator.cpp`;
  - `Jamfile`, `tools/build.mjs` (`--sessions`, read by the render);
  - `pages/examples.oui`, `pages/javascript.oui` ("Running the same code on a server"),
    `pages/cpp.oui` ("Building for each target");
  - `seo/queries.json` ("WASI C++", "C++ wasmtime", "WebAssembly containers C++").

**Interfaces:**
- Consumes:
  - Task 17's calculator: `evaluate(text) -> {ok, value, error}` in C++, from
    `examples/calculator/evaluate.hpp`, which includes `webcpp_site/trace.hpp`;
  - `webcpp::wasi::http::response` (`/webcpp/wasi//wasi`) and the macros (`/webcpp/wasi//http`);
  - the site's own lanes, `webcpp.serve-native` and `serve-words` (Task 31);
  - Boost.Beast and Boost.URL, by R9's branch that Task 20 recorded.
- Produces:
  - **`answer`:**

| Request | Answer |
| --- | --- |
| `GET /?expr=<percent-encoded>` | 200 `text/plain`, the value and a newline |
| a `GET` whose expression does not parse | 400 `text/plain`, pratt's message and a newline |
| a `GET` whose query has no `expr`, or more than 4096 bytes in it | 400 `text/plain`, saying which |
| a query that does not decode to UTF-8 | 400 `text/plain` |
| a path other than `/` | 404 `text/plain` |
| any other method | 405 `text/plain` |

  - **`native_host`:**
    - `native_host --addr 127.0.0.1:<port>`, 0 for any. It prints the `Serving HTTP on ...`
      line on standard error.
    - It answers chunked, so that its transcript holds, as wasmtime's does, no
      `content-length`.
    - It refuses a header block over 8 KiB (Beast's `header_limit`) with 431, and a body of
      more than 0 bytes with 400; `Content-Length: 0`, which `http.client` sends with a
      `POST`, is no body, so such a `POST` reaches `answer` and gets its 405 (M10).
  - **`calculator_http.expected`:** one transcript, checked by
    `webcpp.serve calculator_http.cpp` (the test `calculator_http`, wasip2 and wasip3) and by
    `webcpp.serve-native native_host.cpp : ... : calculator_http` (the test
    `calculator_http_native`, native), in the site's own lane `http`.
  - **`tools/sessions.mjs`:**
    `--transcript <expected> --serve "<words>" --title "<title>" --out <session.json>` writes,
    into the build directory (`<build>/sessions/on-the-server.json`, M8), which `build.mjs
    --render --sessions <dir>` reads; nothing is written into the source tree:

```json
{
    "title": "As a WASI component, served by wasmtime",
    "elided": ["date", "transfer-encoding"],
    "steps": [
        {"command": "wasmtime serve -Scli --addr 127.0.0.1:8080 calculator_http.wasm", "output": "Serving HTTP on http://127.0.0.1:8080/"},
        {"command": "curl -i -X GET 'http://localhost:8080/?expr=1%2F3*3'", "output": "HTTP/1.1 200 OK\ncontent-type: text/plain\n\n1\n"}
    ]
}
```

    - Each later step is a `$ ` line of the transcript, as `serve.py` wrote it, and what
      follows it.
    - The first step's command is the words `serve-words` gives, with `--addr 127.0.0.1:8080`
      and the component's name; its output is `Serving HTTP on http://127.0.0.1:8080/`, the
      line wasmtime prints for that address (M7). For the native host, `./native_host --addr
      127.0.0.1:8080` and the same line.
    - `elided` names the host headers the transcript leaves out, which the page says.
  - **`Terminal(sessions)`:** a list of sessions, each replayed one character at a time from
    the reader's Play, with Pause and Skip, and a line under each saying which headers were
    elided. Under `prefers-reduced-motion` it shows whole. It is text in a `<pre>` from the
    first paint, so that a crawler reads the session.

- [ ] **Step 1: Write the failing tests.**
  - **`answer_test.cpp`** (`webcpp.run`, native, wasip2 and wasip3), each row of the table
    above, plus Review Focus 5:
    - an `expr` of 4,097 bytes, one of 6,000 and one of 100 KB (each 400);
    - `%FF%FE` (400);
    - `1/3*3` (`1`);
    - `2^10` (`1024`);
    - `POST` (405).
  - **`calculator_http.requests`:** `GET /?expr=1%2B2`, `GET /?expr=1%2F3*3`, `GET /?expr=2%2B`,
    `GET /?expr=%FF`, `GET /?expr=<4,097 digits '1'>`, `GET /?expr=<6,000 digits '1'>`,
    `GET /nowhere`, `POST /`. The two long lines are written by `python3 -c` in Step 1 and
    committed as they are: the transcript then proves that both hosts answer them alike (M10).

    Write `calculator_http.expected` by hand only as the test's first input. Then the served
    tests, run once, say whether they agree. Fix the code, never the transcript, until they
    do. The transcript is reviewed as the evidence it is.
  - **`test/sessions_test.mjs`:**
    - over a fixture transcript, the steps are its commands and outputs, byte for byte;
    - a transcript with no `$ ` line fails, naming the file;
    - the first step's command is the words given, with the address; its output names the
      same address;
    - nothing is written outside `--out`.
  - **`test/terminal_test.mjs`:**
    - the server render holds every command and output as text, and the elided headers'
      line;
    - Play, Pause and Skip are `<button>`s with `aria-label`s.
- [ ] **Step 2: Write the Jamfile, both R9 branches spelled, the one Task 20 recorded
  uncommented:**

```jam
import webcpp ;
webcpp.targets native wasip2 wasip3 ;

# R9, Boost.URL, as Task 20 recorded. With /webcpp//boost_url in the superproject:
local url = <library>/webcpp//boost_url ;
# Without it, Boost.URL's sources compiled once, in a library every program here links:
#   lib boost_url_src : boost_url.cpp : <link>static ;
#   local url = <library>boost_url_src ;

# Every program: Boost.URL, pratt, the site's trace header (evaluate.hpp includes it) and the
# calculator's evaluate.hpp.
local common = $(url) <library>/webcpp/pratt//pratt <include>../../include <include>../calculator ;

webcpp.run answer : answer_test.cpp : $(common) <library>/webcpp/wasi//wasi ;
webcpp.serve calculator_http.cpp : $(common) <library>/webcpp/wasi//http ;
webcpp.serve-native native_host.cpp : $(common) <library>/webcpp/wasi//wasi : calculator_http : native ;
webcpp.lane http : answer calculator_http calculator_http_native : native wasip2 wasip3 ;
```

  The site is in no library's ordinary lanes, so `answer` runs in the own lane `http` with the
  served tests, on each of its three targets.
- [ ] **Step 3:** Run each. Expected: FAIL.

```bash
$B2 -a --build-dir=bin/b-native site/examples/on_the_server//http
$B2 -a --build-dir=bin/b-p2 toolset=clang-wasip2 testing.launcher=wasmtime site/examples/on_the_server//http
$B2 -a --build-dir=bin/b-p3 toolset=clang-wasip3 testing.launcher=wasmtime site/examples/on_the_server//http
npm --prefix site test
```

- [ ] **Step 4: Implement.**
  - `answer.hpp`, on Boost.URL's `parse_origin_form` and `params()`, then pratt through
    `evaluate`.
  - The two mains, and `boost_url.cpp` (`#include <boost/url/src.hpp>` alone) under R9's
    second branch.
  - **`b2 site`** writes three sessions from the committed transcript and the words into its
    build directory, through `npm run build -- --sessions`:
    - wasip2 under wasmtime;
    - native;
    - a third, the build commands, from the Jamfile's `tag::` regions.
  - **The page:**
    - the three places, side by side from 1280 px and as tabs on a phone;
    - the browser's leg is the calculator, live, from Task 17's wasm;
    - the WASI leg and the native leg are the sessions;
    - the C++ pane shows `answer.hpp` with the two mains.
  - **The explanation:**
    - one handler and two mains;
    - why the transcripts are the same, and which host headers they leave out;
    - how to run each, with the commands from the Jamfile;
    - wasmtime 47 as the prerequisite;
    - that the native host moves to webcpp::beast when that library exists (Q1).
- [ ] **Step 5:** Run the four commands of Step 3. Expected: PASS. The same `.expected` holds
  on wasip2, wasip3 and native, and `answer` passes on all three.
- [ ] **Step 6:** Add the browser scenario: the page loads, the sessions replay to their end
  on Play, and the calculator leg answers `1+2` with `3`. Run the browser lane. Expected:
  PASS. Look at the page at the four widths, light and dark; fix every fault.
- [ ] **Step 7: Commit** (site): `"On the server: one handler served by wasmtime as a WASI
  component and natively, one transcript checked on all three, replayed as terminal
  sessions"`.

## Task 33: C++ calls the browser (last)

**Files:**
- Create (site), in `examples/cpp_calls_browser/`: `guess.cpp`, `page.ts`, `Jamfile`.
- Create (site): `test/guess_test.mjs`, `test/guess_run.mjs`,
  `pages/examples/cpp-calls-the-browser.oui`,
  `pages/examples/cpp-calls-the-browser/explained.oui`.
- Modify (site): `pages/examples.oui`, `pages/javascript.oui` ("Exporting JavaScript functions
  to C++"), `seo/queries.json` ("call JavaScript from C++").

**Interfaces:**
- Consumes:
  - from webcpp::asio: `io_context`, `browser::async_await` (its Task 17),
    `browser::async_wait_event` (its Task 18), `steady_timer`;
  - `webcpp.variants` and its manifest (its Task 3), and the loader `load(manifestUrl,
    options)` (its Task 12);
  - `traced`, `WEBCPP_SITE_TRACE_SCOPE` (Task 15).
- Produces:
  - **The program `guess`,** declared with `webcpp.variants guess : guess.cpp :
    <library>/webcpp/asio//asio <include>../../include <define>WEBCPP_SITE_TRACE`, which
    writes `guess.variants.json` and both variants.
  - **Its `main`:**

```cpp
int main() {
    webcpp::asio::io_context io;
    const emscripten::val page = emscripten::val::module_property("page");
    boost::asio::co_spawn(io, play(io.get_executor(), page), boost::asio::detached);
    boost::asio::co_spawn(io, clock(io.get_executor(), page), boost::asio::detached);
    boost::asio::co_spawn(io, stop_on(io, page), boost::asio::detached);
    io.run();  // lint-run: the page's event loop, suspended into the browser's
    // Under Asyncify a main that suspended loses its return value, and node exits 0 once no
    // handle is left: the page is told positively that run returned.
    page.call<void>("ended");
}
```

  - **The functions the page exports to C++** (`page.ts`), each wrapped by `traced` as the
    module's imports:

| Function | Does |
| --- | --- |
| `pick(low, high) -> number` | a number from `crypto.getRandomValues` |
| `say(text)` | appends a line to the game's log |
| `ask(question) -> Promise<number>` | resolved by the form's submit, rejected by Give up |
| `tick(seconds)` | shows the time since the game began |
| `newGame` | the New game button, an `EventTarget` C++ awaits with `async_wait_event(..., "click")` |
| `stop` | an `EventTarget` the page dispatches `stop` on when it leaves (`pagehide`) or a test ends the run |
| `ended()` | called by `main` after `run` returns: the program has ended (N2) |

  - **`play`:** picks, then asks in a loop, answering "Higher." or "Lower." by `say`; after a
    win or a rejection, awaits `newGame`'s click and plays again. Every `co_await` uses
    `as_tuple(use_awaitable)`, so that nothing throws on either variant.
  - **`clock`:** a `steady_timer` each second calls `tick`.
  - **`stop_on` (I10):** awaits `stop`'s event, then cancels `play`'s and `clock`'s pending
    operations through a cancellation signal both are bound to, and calls `io.stop()`, so
    that `run` returns and `main` ends.
  - **Trace scopes:** `play`, `round` (with the guess and the comparison's result) and
    `clock`. The trace shows `pick`'s return, the secret, as it shows everything that crosses
    the bridge; the page says so (M9).

- [ ] **Step 1: Write the failing test** `test/guess_test.mjs`. Each variant runs in a child
  process (`test/guess_run.mjs <variant>`), under asio's loader, with a fake `page` whose `ask`
  returns promises the test settles through the child's messages. Each test is bounded by
  `node --test --test-timeout=60000`, and each child is killed, with its process group, when
  its test ends, however it ends:
  - **A full game:**
    - `pick` returns 42;
    - asked 50, 25 and 42, `say` receives "Lower." and "Higher." as they should be, then the
      win with 3 tries;
    - the trace holds, as a subsequence (the `tick` imports interleave in real time, M18),
      `wasm_to_js ask`, then the scope `round` with 50, then `wasm_to_js say ["Lower."]`.
  - **Give up:** the `ask` promise rejects; `say` receives the farewell; the program then
    awaits `newGame`. A dispatched `click` starts a new game (`pick` called again).
  - **Stopped while waiting:** a promise left pending, then `stop` dispatched. The child sets
    `process.exitCode = 1` when it starts and sets it to 0 only in its `ended` import, so a
    `main` that never returned cannot exit 0 (N2). The test asserts:
    - the `ended` crossing (`wasm_to_js ended`) in the trace within 5 s of the `stop`;
    - the child's exit status 0;
    - no handle left (`process.getActiveResourcesInfo()` empty before it exits);
    - its standard error holds no error (Review Focus 5).
  - **Both variants** pass: JSPI where node has `WebAssembly.Suspending`, Asyncify always, as
    asio's loader test does.
- [ ] **Step 2:** Run `npm --prefix site test` after `$B2 -a --build-dir=bin/b-site site`.
  Expected: FAIL, no program.
- [ ] **Step 3: Implement** `guess.cpp`, `page.ts` and the Jamfile.
  - Use the mechanism of Task 13 for the emscripten build under `b2 site`.
  - Both variants and the manifest land in `bin/site/examples/cpp-calls-the-browser/`, and
    both are precached (R15).
  - **The page:**
    - the game's own form and log, with `inputmode="numeric"`;
    - the visualizer;
    - "Variant: JSPI" or "Asyncify", from what the loader picked;
    - `pagehide` dispatches `stop`; `ended` says in the log that the program ended.
  - **The explanation:**
    - functions given to C++ in the module's options;
    - promises and events as Asio operations;
    - why `main` runs the io_context, and how the page ends it;
    - the two variants;
    - the commands from the Jamfile's `tag::` regions.
- [ ] **Step 4:** Run. Expected: PASS, both variants, every child ended.
- [ ] **Step 5:** Add the browser scenarios:
  - a game played through the form to its win;
  - Give up, then New game;
  - the same with `WebAssembly.Suspending` deleted by `Page.addScriptToEvaluateOnNewDocument`:
    the loader picks Asyncify, the page says so, and the game plays.

  Run the browser lane. Expected: PASS. Look at the page at the four widths, light and dark,
  keyboard only; fix every fault.
- [ ] **Step 6: Commit** (site): `"C++ calls the browser: a program on webcpp::asio that drives
  the page through the functions it exports, its promises and its events, and ends when the
  page stops it"`.

## Task 34: The doors and the gallery complete

**Files:**
- Modify (site): `pages/javascript.oui`, `pages/cpp.oui`, `pages/ai.oui`, `pages/index.oui`,
  `pages/examples.oui`, `test/pages_test.mjs`.

- [ ] **Step 1: Write the failing test** in `pages_test.mjs`:
  - every example that exists under `examples/` has a gallery entry linking its live page and
    its explanation;
  - every gallery entry links a page that exists;
  - the roadmap of `examples.oui` and `javascript.oui` names no example that exists, and
    still names file transfer and video chat;
  - the JavaScript door has the four sections of the spec's section 5, each linking a page
    that exists.
- [ ] **Step 2:** Run. Expected: FAIL where a page still lists as coming what exists.
- [ ] **Step 3:** Update the pages. The home's live example above the fold stays the
  calculator.
- [ ] **Step 4:** Run `npm test`, then `$B2 -a --build-dir=bin/b-site site`. Expected: PASS;
  the link check, `sizes-check` and `valid-html` green.
- [ ] **Step 5: Commit** (site): `"The doors and the gallery: every example that exists, and
  only those"`.

## Task 35: Final review and handover

- [ ] **Step 1: Run, each from scratch,** and record each in the ledger:
  - `$B2 -a --build-dir=bin/b-site site`;
  - the site alone: `npm ci`, `npm run dev` (from the preview's files), `npm test`, `npm run
    typecheck`;
  - `$B2 -a /webcpp/site//test`, `$B2 -a /webcpp/site//browser-test`;
  - the site's own lane `http` on native, wasip2 and wasip3 (Task 32 Step 3's commands);
  - trystero's lanes with and without exceptions, its `interop` and `js` lanes, its doc (both
    layouts) and lint;
  - xstate's four lanes;
  - every `tools/**/*_test.py` as `python3 <path>`.
- [ ] **Step 2:** The controller dispatches the whole-branch review (trystero, the site, the
  superproject), on the most capable model, with this plan's Review Focus.
- [ ] **Step 3:** Fix what it finds, each with a failing test first.
- [ ] **Step 4 (controller, G2):** with the owner's agreement, push trystero, the site, then
  the superproject. Watch the CI publish, and open each new page on https://webcpporg.github.io/
  at a phone's width and at a desktop's.
- [ ] **Step 5 (controller):** put the gates still open (G6, and O5 or G7 if they are) to the
  owner, with what each changes.
