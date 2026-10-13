## Context

webcpp::beast is a wrapper of webcpp, not a port. Natively it is Boost.Beast. Compiled with
Emscripten, the same names and interfaces drive the browser's `WebSocket` and `fetch`: the
layers the browser does not expose (resolve, connect, the TLS handshake) record what the browser
needs, and the last one (the WebSocket handshake, or an HTTP request) opens the connection.

The design is `reference/spec.md` (a draft of 2026-10-10, with four decisions of the owner of
that date). It rests on a review of Boost.Beast 1.92 and Emscripten 6.0.11 whose probes are
cited there; this change has not re-run them. The library builds on webcpp::asio
(`libraries/asio`, change `genesis`): the io_context, the bridge that posts the browser's
events to it, Boost's timers, the `ip::tcp` types with the resolver of its decision 5, and the
`ssl` record of the program's TLS settings.

Constraints every decision below works within, from the superproject's rulebook:

- header-only C++20, namespace `webcpp::beast`, macros `WEBCPP_BEAST_*`;
- targets native and Emscripten only, because Boost.Asio 1.92 does not compile for the WASI
  targets with wasi-sdk 34 and webcpp::asio therefore has none;
- errors as values; exceptions optional through `WEBCPP_BEAST_NO_EXCEPTIONS`;
- allocation through allocators the program gives; Boost and std before custom code; outside
  interfaces wrapped; fixed-width types;
- BSL-1.0, "Copyright (c) 2026 WebCpp.org" on every source file; English; no em dash.

## Goals / Non-Goals

**Goals:**

- Beast's three client examples (`websocket_client_async`, `websocket_client_async_ssl`,
  `http_client_async`) run in a browser with the changes the specification names, and print the
  same natively and in the browser.
- Natively, no cost and no difference: Beast's own types, one derived class, two functions.
- In the browser, every Beast setting classified: honoured, unsupported, or ignored with a
  reason; a program learns at the operation what the browser cannot do.
- Bounded memory in the browser: a send bound and a receive queue, both options.
- A defined cancellation, destructor and allocation contract, proven by a counting test.

**Non-Goals:**

- A WASI target (waits for a WASI backend of webcpp::asio).
- `WebSocketStream` (increment 3: Chromium only, doubles the surface behind one interface).
- A resolver of webcpp::beast's own (rejected, decision D4).
- Server-side WebSocket or HTTP in the browser; the browser has no listening socket.
- Ping, pong, client certificates, custom headers other than `Sec-WebSocket-Protocol`, and
  protocol-level fragmentation: the browser does not expose them, and they fail as values.
- Trystero's relays on webcpp::beast (increment 4, a later change).

## Decisions

### D1. A library of its own on webcpp::asio; WebSocket first, then the HTTP client

Owner's decision 1. Beast is a library on Asio, so webcpp::beast is a library on webcpp::asio,
not a part of it. Increment 1 is the WebSocket client on both targets; increment 2 is the HTTP
client in the browser. This change covers both; increments 3 and 4 are later changes.

### D2. Natively it is Boost.Beast; the browser classes exist only on Emscripten

Every name that touches no network (buffers, `http` messages, parsers, serializers, `websocket`
close reasons and errors) is Beast's own on both targets. Natively `tcp_stream` and
`ssl_stream` are Beast's, and `websocket::stream` is a class derived from Beast's, adding the
allocator constructor, so that the two targets have one constructor set. A static-assertion test
proves the aliases are Beast's types.

### D3. What the browser cannot honour fails as a value, by default

Owner's decision 2. Each setting is classified honoured, unsupported, or ignored (only where the
browser does an equivalent thing of its own, with the reason). `WEBCPP_BEAST_UNSUPPORTED_ERROR`
(the default) makes the operation that depends on an unsupported setting complete with
`boost::asio::error::operation_not_supported`, the connection staying as it was;
`WEBCPP_BEAST_UNSUPPORTED_IGNORE` ignores the setting. Silent divergence was rejected: a program
that asks for a client certificate or a ping must learn so.

### D4. The resolver is webcpp::asio's, and its entry is what the browser streams accept

Owner's decision 4 (option A of the review's C4). On Emscripten webcpp::asio's resolver completes
with one entry that carries the host name, the service name and an unspecified endpoint with the
service's port. A plain socket's connect on it fails with `operation_not_supported`, so
webcpp::asio's "fails as a value" holds where a TCP connection would be needed; only
webcpp::beast's browser streams accept it, and Beast's layered client code runs unchanged. A
resolver of webcpp::beast's own, with webcpp::asio's failing like its sockets, was rejected:
every Beast client would change a line.

### D5. The layers record; the last one opens

`async_connect` records the host and service, the TLS handshake judges the recorded settings and
records the scheme, and `ws.async_handshake(host, target)` builds the URL and calls
`new WebSocket(url, subprotocols)`. This is the only way to keep Beast's code shape, because the
browser has one call that does resolution, connection, TLS and the upgrade together.

### D6. The browser WebSocket is selected by the next layer's type

`websocket::stream<NextLayer>` over the browser `tcp_stream` or `ssl_stream` is webcpp's
browser WebSocket; over any other next layer (a relay socket, a WebRTC channel's byte stream) it
is Beast's, as natively. `teardown` and `async_teardown` for webcpp::asio's sockets are declared
in the sockets' namespace so that argument-dependent lookup finds them, as Beast's
`websocket/teardown.hpp` asks of a user-defined socket.

### D7. Back-pressure is a queue in the library, not the browser's

`send` never blocks and never refuses; the library bounds `bufferedAmount` (default 4 MiB, an
option) and fails a write past it with `error::send_limit`. Messages arrive whether a read is
pending or not and wait in a bounded queue in the stream's allocator; past the bound the stream
closes and the next read completes with `error::receive_limit`. A queued message does not keep
`run` alive. `WebSocketStream`, which would pause receiving while C++ does not read, is a later
increment.

### D8. The HTTP client reuses Beast's algorithms by being a byte stream

The browser `tcp_stream` accepts the bytes Beast's serializer writes, parses them back with
Beast's `request_parser`, makes one `fetch` per whole request, and serializes the response with
Beast's serializer into the bytes the reads yield (chunked, with `Content-Length` and
`Content-Encoding` left out). webcpp::beast overloads no HTTP algorithm. The cost is one parse
and one serialization per message, for Beast's code unchanged.

### D9. Configuration is read once, in one header, and cannot silently disagree

`<webcpp/beast/config.hpp>` reads `WEBCPP_BEAST_UNSUPPORTED_ERROR`,
`WEBCPP_BEAST_UNSUPPORTED_IGNORE` and `WEBCPP_BEAST_NO_EXCEPTIONS`; both unsupported macros
defined is an `#error`. A b2 feature drives the setting from the usage requirements. The
browser classes live in an inline namespace named after the setting, as Asio tags its own, so
translation units that disagree name different types and fail at link.

### D10. Errors with no counterpart have a category of their own

The browser's `error` event carries no cause, and a failed connection closes with 1006, so
refusal, DNS, TLS and a blocked port are one error. Seven values in the category
`webcpp.beast` are fixed and never reused; every other outcome maps to an existing Beast or Asio
code (`websocket::error::closed`, `connection_reset`, `beast::error::timeout`,
`websocket::error::message_too_big`, `operation_not_supported`).

### D11. Cancellation is by terminal, partial and total; the destructor aborts and unregisters

The browser cannot unsend a message or undo an opening. The table of the specification states,
per operation and per cancellation type, what happens. The destructor completes every pending
operation with `operation_aborted` (posted, so no pending work keeps `run` alive), closes the
browser socket without a code, and removes its registrations from webcpp::asio's bridge.

### D12. An allocator constructor on both targets

Operation state uses the handler's associated allocator, as Beast's does. The stream's own state
(the receive queue, the registrations, the buffered fragments) uses the allocator given to
`stream(std::allocator_arg_t, const Allocator&, Args&&...)`, and `std::allocator<std::byte>`
without it. Natively Beast's own state uses Beast's allocation, which the documentation names.

### D13. `set_server_name` replaces the one line that cannot compile in a browser

Beast's SSL examples set SNI through `SSL_set_tlsext_host_name(native_handle(), host)`; there is
no OpenSSL handle in the browser. Boost 1.92 has no SNI helper, so `set_server_name` is new on
both targets and a program changes that line once.

## Risks / Trade-offs

- **`basic_resolver_results::create` is outside Boost's documented interface** (guarded by
  `GENERATING_DOCUMENTATION` in 1.92), and the resolver depends on it. Mitigation: a probe
  pins it, and the Boost version is pinned. Falls on webcpp::asio's resolver task as well.
- **The browser hides the cause of a failed connection**, so `connection_failed` covers refusal,
  DNS, TLS, a blocked port and CORS-like failures. Accepted: the Web APIs offer no more.
- **Node enforces neither CORS nor mixed content** (measured by the review), so those failures
  are only seen in the headless-browser lane.
- **One parse and one serialization per HTTP message** in the browser. Accepted for Beast's code
  unchanged; a direct path is a later optimisation if measured necessary.
- **The `ERROR` default makes some of Beast's examples fail as written** (`User-Agent`, added
  roots); the specification lists, per example, what changes under each setting. The trade-off
  is honesty over compatibility.
- **Idle-timeout activity** is messages and close frames only, because the browser answers the
  peer's pings itself and shows none; stated in the table of differences.
- **Dependence on a draft of webcpp::asio.** Its decisions (resolver, bridge, `ssl` record) are
  settled by the owner but unimplemented.

## Migration Plan

The library is new: nothing depends on it. A program that uses Boost.Beast changes `boost::beast`
to `webcpp::beast`, `boost::asio` to `webcpp::asio` (for the names webcpp::asio owns), and the
one SNI line to `set_server_name`. Beast's examples change as section 4.6 of the specification
lists. Trystero moves onto the library in a later change.

## Open Questions

- **O1. Header layout.** The public headers and the convenience header are not fixed by the
  specification (the plan will fix them). webcpp::asio's convenience header leaves `ssl` out
  natively because it brings OpenSSL; whether `<webcpp/beast.hpp>` does the same is to decide.
- **O2. The defaults of the receive queue bound** (the send bound is 4 MiB) and the names of the
  two options. To measure and decide in the plan.
- **O3. How a decorator is called** on an empty `websocket::request_type`: whether any other
  field can be told apart from `User-Agent` by name alone, or every non-subprotocol field is
  unsupported (the specification says the latter: confirm with a test).
- **O4. Streaming request bodies.** The specification buffers a request body whole before the
  `fetch`; whether a streaming upload (Chromium only) is wanted is open, and belongs to a later
  change.
- **O5. The Boost proposal** (specification section 10) is made when both libraries have run in
  real programs, trystero first; no work in this change.
