# Tasks

The tasks of `reference/plan.md` (Tasks 20 to 35, continuing the site plan's numbering), none of them
started. Group 0 records the plan's preconditions on other tracks, with those met checked; the met
ones are a local branch, unpublished (`reference/STATUS.md`).

## 0. Preconditions (plan: "Preconditions on other tracks")

- [x] 0.1 Site Tasks 12 to 15 of the earlier plan: xstate on emscripten, the site's wasm running `machines/player.json`, the PWA and its precache list, the trace and the player's sound (local only: `reference/superproject-patches/`, `reference/site-patches/`, `reference/xstate-patches/`)
- [ ] 0.2 Site Tasks 16 to 18 (the visualizer, the calculator example, the site in headless Chrome) and Task 19 (the review of increments 1 to 3 and their publication)
- [ ] 0.3 The shared connection (`app`) integrated into trystero's `main`, and the superproject's `main` pointing to it
- [ ] 0.4 webcpp::asio's Tasks 2 and 3 (`/webcpp//node-test`, names, `webcpp.variants`) in the superproject's `main`; for group 5 its Tasks 1 to 8, 12, 13, 17, 18 and 23 (published as a submodule)
- [x] 0.5 `/webcpp//boost_url` in the superproject's `main` (it is: R9 takes its first branch, link `/webcpp//boost_url`)

## 1. Increment 4: trystero's JavaScript API (plan: Tasks 20 to 25)

- [ ] 1.1 The ledger, the baseline, the preconditions and the rebase onto the shared connection (plan: Task 20)
- [ ] 1.2 The specification of trystero's JavaScript API, its independent review and gate G4 (plan: Task 21)
- [ ] 1.3 `raw_payload` and `js::runtime`, with their tests natively and on emscripten, with and without exceptions, and the counting test (plan: Task 22)
- [ ] 1.4 `webcpp.js-test` in the superproject, with its fixture tests (plan: Task 23, Steps 1 and 2)
- [ ] 1.5 The facade, its fake module and its tests; `index.d.mts`, `package.json`; `bindings.cpp` and `js/Jamfile`; the lane `js`; the package's README (plan: Task 23, Steps 3 to 8)
- [ ] 1.6 The package in the interop lane, `--api`, Trystero's sketch among its scenarios (plan: Task 24)
- [ ] 1.7 The attribute `{webcpp-site}`, trystero's `doc/javascript.adoc`, its differences, roadmap, testing page and README, and `AGENTS.md` chapter 1's `js/` (plan: Task 25)

## 2. Increment 4: the site serves the package, sizes, the cursors (plan: Tasks 26 to 29)

- [ ] 2.1 `b2 site` serves the package at `packages/trystero/` with `source/bindings.cpp`; `packages`, `needPackage`, `typecheck.mjs`, `dev` fetching the package; the JavaScript path works alone (plan: Task 26)
- [ ] 2.2 Sizes, measured by the build: `tools/sizes.mjs`, `SizeTable`, the libraries page's column, `assemble.py --sizes`, `sizes-check` (plan: Task 27; gate O5)
- [ ] 2.3 The provenance of Trystero's sketch archived and recorded; `cursors.js`, `draw.js`, `surface.ts`, `Frame`, `ExampleSurface`, `scopes.ts`, the visualizer's `frame`, the three pages, the gallery and the JavaScript door, `seo/queries.json` (plan: Task 28, Steps 1 to 5)
- [ ] 2.4 The cursors page looked at in Chrome at four widths, light and dark (plan: Task 28, Step 6; the public-relay run after gate G7)
- [ ] 2.5 The network guard and the browser scenarios of increment 4, the wasm's type, the verdicts from scratch, the independent review of increment 4 (plan: Task 29, Steps 1 to 7)
- [ ] 2.6 Gate G7 resolved, and the owner-approved push that publishes increment 4 (plan: Task 29, Step 8; gates G2 and G7)

## 3. Increment 5: the state machine (plan: Task 30)

- [ ] 3.1 The program `state-machine`, `MachineView`, the trace scopes in `app/site_app.cpp`, the pages, `test/state_machine_test.mjs` against XState 5.33.2, the browser scenario (plan: Task 30, Steps 1 to 7)
- [ ] 3.2 xstate's size on the build that ships, after the sizes tool (plan: Task 30, Step 8)

## 4. Increment 5: on the server (plan: Tasks 31 and 32)

- [ ] 4.1 The site's own lanes from `site/examples`, `webcpp.serve-native`, `serve.py --native` and `serve-words`, with their tests in the tools (plan: Task 31)
- [ ] 4.2 `answer.hpp`, the component, the native Beast host, the transcript, `answer_test.cpp`, the lane `http` on native, wasip2 and wasip3 (plan: Task 32, Steps 1 to 5)
- [ ] 4.3 `tools/sessions.mjs`, `Terminal`, the page and its explanation, the browser scenario (plan: Task 32, Steps 4 and 6 and 7)

## 5. Increment 5: C++ calls the browser (plan: Task 33; last)

- [ ] 5.1 `guess.cpp`, `page.ts` and the Jamfile with `webcpp.variants`, both variants precached; `test/guess_test.mjs` and `guess_run.mjs` on both variants (plan: Task 33, Steps 1 to 4)
- [ ] 5.2 The page and its explanation, and the browser scenarios, Asyncify forced (plan: Task 33, Steps 3 and 5)

## 6. Close (plan: Tasks 34 and 35)

- [ ] 6.1 The doors and the gallery complete: every example that exists, and only those (plan: Task 34)
- [ ] 6.2 Every verdict from scratch, the whole-branch review, its fixes, the owner-approved push, and the gates still open put to the owner (plan: Task 35)
