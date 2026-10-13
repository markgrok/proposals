## Why

State machines and statecharts are how web programs keep track of what they are doing: a form, a
connection, a game, a payment, a peer-to-peer session. XState is the most used library for them
in JavaScript, and a machine written for it is plain data (a JSON config) that Stately Studio can
draw and export. C++ had no library that runs XState's machines as XState runs them: the same
config, the same events, the same actions in the same order, the same actors. A team that wants
one machine for a web page and for a native or WebAssembly server, or a program that has to run
machines it did not write and still answer within a budget, had to rewrite the machine for a
compile-time library and give up both the config and XState as its judge. webcpp needs the port
for its own reasons too: its asynchronous libraries are built on actors, and its site runs its
own state machines on webcpp.

## What Changes

- **New library `webcpp::xstate`**, a port of XState 5.33.2's state machines and actors, header-only
  C++20, in two layers:
  - **The machine core**, `<webcpp/xstate.hpp>`: a machine is created from the JSON config
    XState's `createMachine` reads (`create_machine`), with the C++ implementations of its actions,
    guards, delays and actors registered by name (`implementations`, XState's `setup`), and is
    stepped by XState's pure functions (`initial_transition`, `transition`, `get_next_snapshot`,
    `get_microsteps`, `can`, `get_next_transitions`) or, one microstep at a time, by the macrostep
    cursor (`begin_initial`, `begin`, `macrostep::next`). It needs Boost's headers, Boost.JSON and
    webcpp's configuration, and includes nothing of xactor.
  - **The actor layer**, `<webcpp/xstate/actors.hpp>`: the same machines run as XState's actors on
    webcpp::xactor (`actor_system`): invoke and spawn, `sendTo`, `sendParent`, `forwardTo`, `emit`,
    `after`, `systemId`, done and error events, inspection, host actors in place of promises, and
    fuel with parking and resume.
- Every operation that can fail returns a `result<T>` of the category `webcpp.xstate` (ten
  `errc` values); xstate throws no exception of its own, holds no global or static mutable state
  and reads no clock, file or network.
- It is proven against XState itself: the oracle (XState 5.33.2 under Node) runs 504 machine cases
  and 228 actor cases ported from XState's tests, and a twin (the same program written for XState)
  runs for 136 of the 146 example programs, 22 of which print XState's different output on
  purpose, each difference being listed and explained.
- It follows webcpp's conventions: b2 (`build.jam`, `test/Jamfile`, `example/Jamfile`), native,
  wasm32-wasip2 and wasm32-wasip3, Boost.Core's lightweight_test, one documentation page with an
  MrDocs reference, BSL-1.0 with XState's MIT notice kept for what derives from it.

## Capabilities

### New Capabilities

- `xstate`: webcpp::xstate 0.1.0, XState 5.33.2's state machines and actors for C++20.

### Modified Capabilities

None.

## Impact

- **Kind:** a port (`"port-of": XState 5.33.2`, TypeScript, MIT).
- **Implementation:** https://github.com/webcpporg/xstate at `5299db2`, the submodule `libs/xstate`
  of the superproject; `reference/source/` is that tree (680 files). The code was written in an
  earlier private project and moved here on 2026-10-07 as one commit, `631e737`, then converted to
  webcpp's conventions in the 26 commits that follow.
- **Dependencies:** webcpp::xactor (the actor layer only, through `/webcpp/xactor//xactor`),
  webcpp::config, Boost 1.92's headers (Boost.JSON, Boost.System; Boost.Asio through xactor off
  WASI) and Boost.JSON's compiled definitions (`/webcpp//boost_json`). For the oracle lane only:
  Node and npm, with the npm package `xstate` pinned at 5.33.2.
- **Status:** implemented, in release 0.1.0 (`reference/STATUS.md`).
- **Other proposals:** depends on the genesis of `xactor` (the actor layer is built on it, and the
  machine core is checked to include none of it). webcpp's own site is planned to run its state
  machines on this library compiled to WebAssembly.
