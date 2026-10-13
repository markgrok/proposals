# webcpp::webrtc: where the proposal stands, and how to resume

Written 2026-10-13, when this proposal was drafted from the specification.

**Reference code: none yet; required before the proposal is accepted.**
**Plan: none yet; required before the proposal is accepted.**

## What exists

- `spec.md`: the draft specification, dated 2026-10-10 (status `draft`): the owner's decisions of
  that day, the controller's rulings on the review, the interface (section 4), the semantics
  (section 5), exceptions, what a target cannot honour, testing, libdatachannel, the three
  increments and the questions open for the owner. It is the only file the proposal's
  requirements and tasks rest on.
- No repository, no branch, no header, no test, no commit. Nothing was built, run or measured
  for this library.

## Measured

Nothing. The draft quotes facts about libdatachannel 0.24.6 (`RECV_QUEUE_LIMIT` of 1,024,
`DEFAULT_LOCAL_MAX_MESSAGE_SIZE`, the callbacks' recursive mutex, `resetCallbacks()`), about
Chromium's limit of 500 peer connections per page and about two Chrome switches, each with the
file and line it read them from. None was re-verified when this proposal was written. Two
measurements are left to increment 1: whether a never-read native channel stalls the
connection's other channels, and the allocation behaviour on the executor.

## The nearest existing code

trystero's `backend` interface with `native/peer_connection.hpp`, `native/data_channel.hpp` and
`browser/backend.hpp` is this library's first draft, section 3 of the specification maps every
part of it. It lives in trystero's own proposal (`libraries/trystero`, its `reference/source/`),
and it is not this library's reference code: it is a virtual interface with callbacks and no
runtime, bounds or error category.

## Open

- The plan (`reference/plan.md`) and the reference code (tasks.md, group 0).
- The owner's two decisions: libdatachannel's MPL-2.0 licence, and whether a native program
  built without exceptions gets the compiler's message or a stub's (design.md, Open Questions).
- The names of headers, Jamfile targets and the repository layout, which the plan fixes.
- webcpp::asio's browser bridge and suspension variants, which increment 2 consumes; that
  proposal is a draft too.

## How to resume

1. Read `spec.md` whole, then `../design.md` and `../tasks.md`; read webcpp::asio's genesis
   (`libraries/asio`: executors, the bridge, JSPI and Asyncify) and trystero's genesis
   (`libraries/trystero`: the `backend` this library replaces).
2. Do tasks 0.1 and 0.2: write `reference/plan.md` in the three increments of section 10, and
   the reference code for the interface and the native implementation, with the tests of
   section 8 that apply natively. Update this file with the repository, the commit and what was
   measured, and when.
3. Ask the owner for the decisions of section 11, then propose the change to `proposed`.
4. Implement increment 1, then 2, then trystero's change (increment 3), in that order.

Prerequisites are the superproject's: Boost 1.92 with its sources, b2, a C++20 compiler,
Emscripten from emsdk for increment 2, libdatachannel 0.24.6 found through
`-sLIBDATACHANNEL_ROOT=<dir>` or the superproject's `.local/libdatachannel`, and a headless
Chrome for the interop lane.
