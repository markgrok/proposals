## Why

Code written against Boost.Beast (resolve, connect, the TLS handshake, the WebSocket handshake,
then reads and writes) cannot run in a browser: the browser has no sockets, only `WebSocket` and
`fetch`. A C++ program that talks WebSocket or HTTP, and that must run natively and on
Emscripten, therefore has two code paths today, or a custom abstraction over both. webcpp wants
one: Beast's own layered code, unchanged, driving the browser's `WebSocket` and `fetch` when
compiled with Emscripten, and Boost.Beast itself natively. It is to webcpp::asio what Boost.Beast
is to Boost.Asio, and it is what trystero's relays (WebSocket signalling) need to stop polling
the browser from event handlers. With webcpp::asio it is also the prototype of a later proposal to
Boost.

The library is specified (draft of 2026-10-10, with four decisions of the owner) and has no plan
and no code yet. This change records the specification as a proposal, so that the plan and the
reference code are the first work and everything after them is traceable to a requirement.

## What Changes

- **New library `webcpp::beast`**, a wrapper (not a port) over Boost.Beast and webcpp::asio,
  header-only C++20, on native and Emscripten (webcpp::asio has no WASI target, so neither has
  this library until a WASI backend exists).
- **Natively** it is Boost.Beast: aliases of Beast's types, a `websocket::stream` derived from
  Beast's that adds an allocator constructor, `set_server_name` (the one line of Beast's SSL
  examples that cannot compile in a browser), and `teardown` / `async_teardown` for
  webcpp::asio's sockets.
- **On Emscripten** the same names drive the browser: `tcp_stream` and `ssl_stream<tcp_stream>`
  are browser streams that record what the layers say (resolver entry, endpoint, TLS settings)
  and `websocket::stream` over them opens `new WebSocket(...)` at the handshake, with reads,
  writes, close, timeouts, back-pressure queues, cancellation and a destructor contract.
- **What the browser cannot honour fails as a value** (`boost::asio::error::operation_not_supported`)
  by default, `WEBCPP_BEAST_UNSUPPORTED_ERROR`; a program may define
  `WEBCPP_BEAST_UNSUPPORTED_IGNORE` to have it ignored. A b2 feature,
  `webcpp-beast-unsupported=error|ignore`, drives it for a whole program, and the browser classes
  live in an inline namespace named after the setting so that disagreeing translation units fail
  at link.
- **Errors by name:** failures with no counterpart in Beast or Asio are in the category
  `webcpp.beast` (`invalid_url`, `blocked`, `connection_failed`, `send_limit`, `receive_limit`,
  `fetch_failed`, `no_host`), with fixed values never reused.
- **The HTTP client in the browser (second increment):** the browser `tcp_stream` is a byte
  stream, so `http::async_write` and `http::async_read` run unchanged over it; each whole request
  becomes one `fetch`, and the response is serialized back into the bytes the reads yield.
- **The rules hold from the first commit:** exceptions on and off, both Emscripten suspension
  variants, allocators, Boost first, outside interfaces wrapped, fixed-width types.
- Tests: native lanes, Emscripten lanes under node on both variants, a headless-browser lane,
  native Beast echo servers (plain and TLS), and a twin of each guide example that prints the
  same natively and in the browser.
- Out of this change (later increments, later proposals): `WebSocketStream` where the browser has
  it, and trystero's relays on webcpp::beast.

## Capabilities

### New Capabilities

- `beast`: webcpp::beast, Boost.Beast's names and layered interfaces natively, and over the
  browser's `WebSocket` and `fetch` on Emscripten, with its unsupported-setting policy, error
  category, back-pressure, cancellation, allocation, configuration, tests and documentation.

### Modified Capabilities

None.

## Impact

- **Kind:** a wrapper (`port_of: null`): webcpp's interface over Boost.Beast and, in the
  browser, over the Web APIs. Repository `webcpporg/beast` (not created), checked out as the
  submodule `libs/beast`, BSL-1.0, "Copyright (c) 2026 WebCpp.org".
- **Implementation:** not started. There is no reference code: `reference/` holds the
  specification alone, and a plan and the reference code are required before the proposal is
  accepted. They are the first tasks (`tasks.md`, group 0).
- **Dependencies:** webcpp::asio (the proposal `libraries/asio`, change `genesis`: the
  io_context, the bridge that posts the browser's events, the `ip::tcp` types and resolver, the
  `ssl` record); Boost.Beast 1.92 and Boost.Asio, Boost.System; OpenSSL natively (through
  Boost.Asio's `ssl`); webcpp::config; Emscripten 6.0.11 with its two suspension variants; node
  26 and headless Chrome for the lanes.
- **Status:** draft; no release. Targets are native and Emscripten only.
- **Other proposals:** depends on webcpp::asio's genesis (its Emscripten resolver of decision 4,
  its bridge and its `ssl` record must exist first). The later trystero change that moves its
  relays onto webcpp::beast depends on this one.
