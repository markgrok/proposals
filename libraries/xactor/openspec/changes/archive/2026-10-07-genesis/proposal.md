## Why

A program that runs many cooperating pieces of logic (a protocol session, a state machine for each
connection, a timer for each deadline) wants to reason about each piece alone, test it by the
messages it takes and sends, and know that the whole run is repeatable and ends. C++ has threads,
Boost.Asio handlers and coroutines, but no small actor system that promises three things together:
the same deliveries make the same run, whichever executor drives it; every chain or storm of
messages is bounded by a budget; and time is a number the host brings, so a test moves it by any
amount at once. It must also run unchanged natively and on WebAssembly (wasm32-wasip2 and
wasm32-wasip3), where there is no thread to lean on and Boost.Asio does not compile.

webcpp needs this layer first: every asynchronous library it ports or writes is to be built on an
actor system, and webcpp::xstate (state machines and their actors) runs its actors on it. xactor
therefore stands alone, generic over the program's message type, and knows no other webcpp library.

## What Changes

- **New library `webcpp::xactor`**, webcpp's own (not a port), header-only C++20, in the namespace
  `webcpp::xactor`, one convenience header `<webcpp/xactor.hpp>` and ten headers under
  `<webcpp/xactor/>`: `config`, `errors`, `ids`, `status`, `budgets`, `envelope`, `envelope_log`,
  `actor_logic`, `scheduler` (with `turn` and `create_actor`) and `drivers`.
- An actor is a class derived from `actor_logic<Message>` whose one `handle` takes the actor's
  `turn` and the `envelope` of one message at a time. The `Message` is the program's closed
  `std::variant`, which only has to be movable.
- A `scheduler<Message>` gives each actor a FIFO mailbox and keeps one FIFO queue of the actors with
  work, so the order of deliveries depends on the order of enqueuing alone. It starts no thread and
  reads no clock.
- Every execution (everything one host delivery causes, named by a `correlation_id`) is paid from a
  budget of fuel: one unit for each message sent and timer armed, and `spend(n)` for an actor's own
  work. What cannot be paid fails with `errc::resource_limit` and changes nothing.
- Timers are armed with `wake_at` (optionally under a key) and released only when the host calls
  `clock_tick` or `release_next` with a time that reaches their deadline.
- Actors have a lifecycle (`active`, `done`, `error`, `stopped`, with fixed values 1 to 4), parents
  and children, and a family is stopped descendants first.
- Three drivers decide when turns happen: `fifo_driver`, `test_driver` and, native only,
  `asio_driver`. All of them record the same `envelope_log` for the same deliveries.
- Every operation that can fail returns `result<T>` (Boost.System) with an error of the category
  `webcpp.xactor`; the library throws nothing of its own and its headers compile without exceptions
  (`WEBCPP_XACTOR_NO_EXCEPTIONS`).
- Proven by four lightweight_test programs (55 cases, converted from an earlier suite with an
  equivalence record), a `config` compile test built three ways, a compile of every header alone, eight example
  programs whose output is compared with `.expected` files, on native, wasip2 and wasip3; and
  documented by a page of 36 numbered guarantees and a strict MrDocs reference.

## Capabilities

### New Capabilities

- `xactor`: webcpp::xactor 0.1.0, a deterministic actor system for C++20.

### Modified Capabilities

None.

## Impact

- **Kind:** a library of webcpp's own (`"port-of": null`); its code was extracted from the actor
  layer of an earlier private project, renamed, re-tested and re-documented as a library of its own.
- **Implementation:** https://github.com/webcpporg/xactor at `62f3e77`, the submodule `libs/xactor`
  of the superproject; `reference/source/` is exactly that tree.
- **Dependencies:** Boost's headers 1.92 or newer (Boost.System for `result` and `error_code`;
  Boost.Asio for `asio_driver` outside WASI; Boost.Core's lightweight_test for the tests; Boost.MSM
  for one example) and webcpp::config (`/webcpp/config//config`). Nothing is compiled or linked.
- **Status:** implemented and published in release 0.1.0 (`reference/STATUS.md`).
- **Other proposals:** webcpp::xstate builds its actor layer on xactor; later work on shared or
  relocatable actor state is not specified here.
