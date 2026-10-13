## Why

Boost.Asio is the ground of asynchronous C++: Boost.Beast, Boost.Cobalt, Boost.MySQL, Boost.Redis
and many other libraries are written on it, as templates over its executors, its completion
tokens and its stream concepts. None of that runs in a browser today in the form a C++ programmer
writes it: a program with `main()` and `io_context::run()` has no way to wait for a timer, a
message or a promise without blocking the page, and a page that is never given back to the
browser stops rendering. Emscripten 6.0.11 can suspend `io_context::run` into the browser's event
loop, but Boost.Asio does not build there without a configuration, nothing wakes a suspended
`run` from a browser event, and a self-posting handler starves the page (measured: 300,000 runs
before a `setTimeout(0)` ran).

webcpp needs a thin layer that closes exactly those gaps and keeps everything else Boost's, so
that a library written on webcpp::asio (webcpp::trystero first, then webcpp::beast and
webcpp::webrtc) and every Boost library built on Asio never knows where it runs. The browser's
own asynchronous model (promises, events) should reach C++ as ordinary Asio operations too.

## What Changes

- **New library `webcpp::asio`**, a wrapper (webcpp's interface over Boost.Asio, not a port):
  header-only C++20 over Boost.Asio on two targets, native and Emscripten.
  Natively every name is Boost's. On Emscripten it is Boost's own `io_context` suspended into
  the browser's event loop. There is no WASI target (Boost.Asio 1.92 does not compile for
  wasm32-wasip2 or wasm32-wasip3 with wasi-sdk 34).
- `webcpp::asio::io_context`, derived from `boost::asio::io_context` on both targets, with
  `io_context_options{max_handlers_per_turn}` (1000 by default), a reactor made at construction
  and a run family composed of `poll_one` and `run_one` that yields to the browser through a
  `MessageChannel` macrotask after every turn on Emscripten; timers are aliases of Boost's.
- The bridge, `webcpp::asio::browser::bridge`, a service of the io_context through which browser
  events only post; and the detection that refuses a run family call made from a handler or from
  a delivery.
- Two suspension variants built side by side from the same sources, `jspi` (default, WebAssembly
  exceptions) and `asyncify` (JavaScript exceptions, a measured stack, `asyncify-stack-size`),
  with names Boost's way, a manifest and a loader that picks the variant a browser runs.
- Boost.Cobalt's `co_main` given the turn limit by webcpp's own `run_main`; JavaScript promises
  and DOM events as Asio operations (`async_await`, `async_wait_event`); sockets and a resolver
  that fail as values behind a transport point; the `ssl` names.
- Superproject changes the library needs: the propagated composite feature `webcpp-suspend`,
  a test support that fails a test whose `main` did not finish (`webcpp::test::exit_status`),
  `webcpp.variants`, `webcpp.size-table`, and `/webcpp//boost_cobalt`.
- No breaking change: nothing depends on asio yet.

## Capabilities

### New Capabilities

- `asio`: webcpp::asio, Boost.Asio natively and in the browser Boost.Asio's own io_context
  suspended into the page's event loop, with its io_context, run family, turn limit, bridge,
  detection, suspension variants, Asyncify stack, loader, Cobalt `run_main`, promises and
  events as operations, transport point, resolver, `ssl` names, allocation and exception
  guarantees, tests, examples and documentation.

### Modified Capabilities

None.

## Impact

- **Kind:** a wrapper (`"port-of": null`): webcpp's interface over Boost.Asio, repository
  `webcpporg/asio` (not created; the owner creates it, gate G1), checked out as the submodule
  `libs/asio`, BSL-1.0, "Copyright (c) 2026 WebCpp.org".
- **Implementation:** a local repository, unpublished, 13 commits on `main` of 2026-10-11 (tip
  `7cfd449`), all as `R`, covering tasks 1 and 4 to 9 of the plan; the superproject's tasks 2 and
  3 are 10 commits on a superproject branch, also local. `reference/source/` is the library's
  tree at the tip; `reference/STATUS.md` says how the two local copies differ and how to resume.
  Tasks 10 to 23 are not started.
- **Dependencies:** Boost 1.92 headers (Asio, and for the tests Beast's `test::stream`;
  Cobalt's sources are compiled by the superproject, task 15; Core's `lightweight_test`;
  Unordered's `unordered_flat_map` for the bridge), Emscripten 6.0.11 with embind, node 26.7.0 for
  the tests, chrome-headless-shell for the browser lane, OpenSSL 3 natively and only for
  `<webcpp/asio/ssl.hpp>`. webcpp::xactor is a user of the library, not a dependency. No new
  third-party code.
- **Status:** `implementing`; not in a release; the owner accepted the specification on
  2026-10-11 and approved the plan's five departures the same day.
- **Other proposals:** webcpp::beast (WebSocket and HTTP, over the bridge, the transport point
  and the `ssl` record), webcpp::webrtc (over the bridge) and trystero's migration onto asio
  (its event-side polling removed) depend on this one; none exists as a proposal yet.
