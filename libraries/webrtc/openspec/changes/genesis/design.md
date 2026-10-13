## Context

webcpp::webrtc is a header-only C++20 library for peer connections and data channels, shaped as
the W3C WebRTC API and written in Asio's model. Its design is in `reference/spec.md`, a draft
dated 2026-10-10 with the owner's decisions of that day (section 2.1) and the controller's
rulings on a review (section 2.2). There is no plan and no reference code yet. The nearest
existing code is trystero's `backend` interface and its native and browser implementations (in
trystero's own reference): a virtual interface with handler callbacks, one implementation per
target, which this library replaces with value types and asynchronous operations.

Constraints every decision works within, from the superproject's rulebook (its `AGENTS.md`):

- header-only C++20, namespace `webcpp::webrtc`, headers under `<webcpp/webrtc/...>`, internals
  in `detail`, macros `WEBCPP_WEBRTC_*`;
- targets native and Emscripten only (D3); every test on each target unless its Jamfile says why
  not;
- errors as values; exceptions optional through `WEBCPP_WEBRTC_NO_EXCEPTIONS`, with the native
  backend unavailable without them (D4);
- no global or static mutable state, so process-wide platform state is held by an object the
  program owns (D6);
- allocation through the allocator the program gives, on the executor (D9);
- Boost and std before custom code; third-party code only as a pinned dependency by the
  owner's decision; BSL-1.0, "Copyright (c) 2026 WebCpp.org" on every source file; English; no
  em dash.

## Goals / Non-Goals

**Goals**

- One interface for peer connections and data channels that a program writes once and runs
  natively and in a browser, with the W3C API's names and shapes.
- Operations in Asio's model: completion tokens, handlers on the object's executor, terminal
  cancellation, outstanding work, the execution context outliving the object.
- Events as pending operations with stated policies, so that no event is lost silently and every
  lifetime stays in the caller's hands.
- Every limit webcpp imposes is a named option and fails as a value; every platform limit is
  stated in a table of differences.
- A native peer and a browser peer connect to each other in the tests.
- trystero can be rewritten on it without loss (spec section 3 maps every part of trystero's
  `backend`).

**Non-Goals**

- Media: tracks, codecs, transceivers. Media will get an interface of its own.
- wasip2 and wasip3 targets (no WebRTC stack exists for WASI). A later WASI target is a change
  of its own.
- `createOffer`, `createAnswer` and the `negotiationneeded` event (D12).
- A native handle (`rtc::` or `emscripten::val`) in any public header.
- Zero-copy receive, for now (D8).
- Policies of trystero's: dropping text messages, dropping frames larger than a room frame, the
  per-minute window of connection attempts.
- An integration with xactor inside the library (D11).

## Decisions

### D1. Kind: own, not wrapper

The metadata says `kind: own`. On the native target the library does wrap libdatachannel, and
the owner's decision 2.1.3 says so, but a wrapper in webcpp's sense is "webcpp's interface over
another library", whose design is that library's with a thinner face. Here the interface, the
semantics and the bounds are webcpp's: the event policies, the `runtime`, the error category, the
back-pressure rules and the table of differences are specified independently of libdatachannel,
and the second implementation is not a library at all but the browser's objects. The owner's
decision 2.1.1 is "a library of its own, on webcpp::asio". There is no upstream whose tests prove
the library (so not `port`). libdatachannel is justified in the documentation as a dependency of
one backend (D14).

### D2. The W3C API's shape and names, in Asio's model

Types and operations carry the W3C names (`peer_connection`, `data_channel`,
`session_description`, `ice_candidate`, the `RTC*` enumerations in snake case, `ready_state()`,
`buffered_amount()`, `close()`). Operations are `async_*` members taking any completion token;
the executor and the allocator are template parameters defaulting to
`boost::asio::any_io_executor` and `std::allocator<std::byte>`, as Asio's own I/O objects do.
Inputs are views read during the call; outputs are owned, with their text in
the object's allocator.

### D3. Targets: native and Emscripten

wasip2 and wasip3 are not targets. libdatachannel needs threads, which neither WASI target
gives, and a UDP socket layer and a DTLS stack that it has no WASI port of; Boost.Asio itself
does not build for those targets with the pinned wasi-sdk. The build declares native and
emscripten only, and the convenience header stops with `#error` under `__wasi__`, naming this
reason. Rejected: a stub for WASI (it would let a program compile and fail at run time).

### D4. Exceptions: the native backend needs them

libdatachannel reports errors by throwing. The native backend calls it only where exceptions are
on. Without them (`WEBCPP_WEBRTC_NO_EXCEPTIONS`) the native backend is not available, and a
program learns it at compile time by the superproject's rule for a header that wraps a throwing
dependency: what follows the native headers' `config.hpp` sits inside
`#ifndef WEBCPP_WEBRTC_NO_EXCEPTIONS`, with a comment naming libdatachannel, so naming
`webcpp::webrtc::peer_connection` fails on that name and merely including the header compiles.
The emscripten backend works with exceptions on and off. Inside the native backend, every call
into libdatachannel that may throw is caught and becomes a value. Rejected for now: a declared
stub whose use fails a `static_assert` with a clearer message, which departs from the rule's
"declares nothing" (Open Questions).

### D5. Events are pending operations the caller re-arms

An event is a wait (`async_wait_ice_candidate`, `async_wait_state_change`,
`async_wait_ice_connection_state_change`, `async_accept_data_channel`, `async_wait_open`,
`async_wait_close`, `async_wait_buffered_amount_low`, and `async_receive`), not a callback. This
keeps every handler on the executor and every lifetime in the caller's hands, as an acceptor's
`async_accept`. While no wait is pending, each kind has a stated policy: state changes coalesce
to the latest value; candidates queue up to `max_queued_candidates` and the end of gathering has
a place the bound keeps for it; incoming channels queue up to `max_queued_channels` and a channel
past the bound is closed at once, as a connection past a listen backlog is refused. One wait of a
kind at a time (a second completes with `already_started`); a cancelled wait consumes no event.
Rejected: one wait returning a variant of events (the review's answer), and callbacks.

### D6. A `runtime` owns the platform's process-wide state

libdatachannel keeps process-wide state (`Preload`, `Cleanup`, `SetThreadPoolSize`,
`InitLogger`), and its threads must be stopped through `Cleanup()` before static destruction.
webcpp keeps no global or static mutable state, so the program owns that state through a
`runtime`: made before any peer connection and destroyed after the last; `make` takes it by
reference, so no connection can exist without one. At most one exists at a time in a process
natively. Its other duty is the cap of peer connections on both targets
(`max_peer_connections`, default 256, half of Chromium's 500 per page, counted from
construction until garbage collection): natively it counts live connections made with it; in the
browser it reads the page's census, which lives in JavaScript. Rejected: a hidden singleton
(static state), and a per-connection cap (it cannot see the page).

### D7. Native handlers are posted, and destruction is safe against libdatachannel's threads

libdatachannel's events arrive on its own threads. Each is posted to the executor, never run
there, and nothing of the user's allocator is touched on those threads: the posts carry Asio's
default allocator and the queues are filled on the executor. Destruction calls `resetCallbacks()`
on the `rtc::PeerConnection` and on every `rtc::DataChannel` before releasing them; that call
synchronises with a callback in flight because each callback runs under its own recursive
mutex, so once it returns nothing more is posted, and what was posted before finds nothing,
through a weak registration. In the browser the object's registration with webcpp::asio's
bridge is removed and the JavaScript handlers are set to null. The execution context outlives
the object, as for Asio's I/O objects, which is what keeps a libdatachannel thread from posting
to a destroyed `io_context`.

### D8. Receiving and sending

`async_receive` appends one whole message to the caller's dynamic buffer, copied once, and
reports whether it was text. No zero-copy receive for now: in the browser the bytes must leave
the JavaScript heap anyway, and natively `rtc::binary` is a `std::vector` in libdatachannel's
allocator, so the one copy is the copy that puts the bytes under the user's allocator. Messages
are kept from the channel's creation. Natively the queue is libdatachannel's own, in its pull
mode (1,024 messages), and past it libdatachannel's thread waits and the peer is slowed: real
flow control, never a failure. In the browser, which has no flow control on receipt, a queue
bounded in bytes (`max_received_bytes`) is the only protection, and past it the channel closes
with `receive_overflow`. `send` and `send_text` queue and return, and `async_send` forms exist
because both of trystero's backends send synchronously, fire and forget, as Asio's sockets offer
`send` beside `async_send`; each fails as a value (`invalid_state`, `buffered_amount_exceeded`,
`message_too_large`). The caller paces itself with `async_wait_buffered_amount_low`.

### D9. Errors, allocation and cancellation

Errors are values: `webcpp::webrtc::error`, category `webcpp.webrtc`, ten enumerators with fixed
values that are never reused (`invalid_state` 1 to `too_many_peer_connections` 10). Asio's codes
keep their meaning: `operation_aborted`, `already_started`, `operation_not_supported`. Every
run-time object (the connection, its channels, their queues, the text of descriptions,
candidates and labels) is allocated through the object's `Allocator`, on the executor; what
libdatachannel and the browser hold is theirs, and none of the types is shareable. Every
operation supports terminal cancellation; a cancelled description operation cannot cancel the
promise (browser) or the libdatachannel thread (native) behind it, so its effect may happen
after the handler ran with `operation_aborted`, and the documentation says so.

### D10. `close()` fires no state change

As the W3C's `close()`: every pending operation of the connection completes with
`operation_aborted`, `state()` reads `closed`, no state-change event is delivered, and the
connection's channels then close with `error::closed` on their pending operations. A data
channel's `close()` completes its pending operations with `operation_aborted`, except
`async_wait_close`, which completes once the channel is closed. When a channel closes by other
causes (the peer, the connection, its transport) its pending operations complete with
`error::closed` and `async_wait_close` completes successfully; trystero ends a link on that.

### D11. How it meets xactor

It does not: webcpp::webrtc knows nothing of xactor. They meet at the executor. An xactor actor
that needs a peer holds a `peer_connection` made on the executor its driver runs on, starts
operations from its turns, and turns each completion into a message to itself.

### D12. Left out of the W3C API: `createOffer`, `createAnswer`, `negotiationneeded`

libdatachannel marks `createOffer` and `createAnswer` "for specific use cases only", and its
`setLocalDescription` takes a type, not a description, so a description created that way cannot
be applied as the W3C allows; the implicit `setLocalDescription`, the one trystero uses, does
both. libdatachannel has no `negotiationneeded` event, only a query. Natively automatic
negotiation is off, so on both targets a description exists only when
`async_set_local_description` asks for one, and the program negotiates when it adds a channel. A
later version may add the event, synthesised natively. Both omissions are in the table of
differences.

### D13. What a target cannot honour follows webcpp::beast's rule

`operation_not_supported` under `WEBCPP_WEBRTC_UNSUPPORTED_ERROR` (the default), ignored under
`WEBCPP_WEBRTC_UNSUPPORTED_IGNORE`. The documentation keeps the table of differences
(`bind_address`, local `sdp_m_line_index` and `username_fragment`, a received message past
`max_message_size`, unread messages, the omitted operations, no native handle).

### D14. libdatachannel, wrapped and justified

The native implementation wraps libdatachannel behind the interface; nothing of `rtc::` appears
in a public header. The documentation justifies it as the superproject's rule asks: no WebRTC
exists in webcpp, Boost, the C++ Alliance or the standard library, and libdatachannel is the
smallest complete implementation (data channels without the media stack), MPL-2.0, with its own
OpenSSL or mbedTLS for DTLS. webcpp uses it unmodified, as a dependency, and copies none of its
files. It is not pinned by webcpp today: like trystero, the library finds libdatachannel 0.24.6
where `-sLIBDATACHANNEL_ROOT=<dir>` or the superproject's `.local/libdatachannel` says, and it
is pinned when webcpp builds it from `third_party/`.

### D15. Tests: every lane on both targets, interop with a real browser

Every lane runs on native and on emscripten, both suspension variants, at the owner's three CI
levels. Each target alone: two peers in one process or one page, every case the semantics name.
Native lifetime: destruction while libdatachannel's threads deliver events, repeated, under
ThreadSanitizer. Interop: a native peer and a headless-Chrome peer, signalled through a local
relay, with Chrome started with `--allow-loopback-in-peer-connection` and
`--disable-features=WebRtcHideLocalIpsWithMdns`, and the native peer binding to `127.0.0.1`.
Allocation: a counting resource. Exceptions: emscripten suites with exceptions on and off;
natively, a headers-alone translation unit and a `webcpp.compile-diagnostic` test. No network
beyond loopback: host candidates only, no STUN.

### D16. Increments

1. The interface, the `runtime` and the native implementation, with their tests.
2. The browser implementation, its tests and the interop lane.
3. trystero on webcpp::webrtc (with webcpp::asio and webcpp::crypto), its `backend` gone.

## Risks / Trade-offs

- [No plan and no reference code] -> the proposal cannot be accepted as it is: writing the plan
  and the reference code is group 0 of the tasks.
- [A native channel that is never read holds one of libdatachannel's threads while its queue is
  full] -> increment 1 measures whether that stalls the connection's other channels, and the
  documentation says what it finds.
- [A cancelled description operation still takes effect] -> stated on every such operation's
  page; the W3C promise cannot be cancelled.
- [The browser has no flow control on receipt] -> `max_received_bytes` closes the channel with
  `receive_overflow`; documented as a difference.
- [Without exceptions the native backend is unavailable] -> by design (D4); a native program
  needing no exceptions cannot use it; the compile-time message is the compiler's own.
- [The compile-time message is unclear] -> accepted; the documentation names it on the library's
  page and in `config.hpp`'s Doc Comment.
- [libdatachannel is unpinned today] -> found through a variable or `.local`; pinned when built
  from `third_party/`; its version (0.24.6) is stated, and constants quoted from it
  (`RECV_QUEUE_LIMIT` 1,024, `DEFAULT_LOCAL_MAX_MESSAGE_SIZE`) must be re-checked when it moves.
- [MPL-2.0] -> file-level copyleft reaches none of webcpp's files while the dependency is used
  unmodified; the owner decides (Open Questions).
- [Interop depends on Chrome's flags] -> the flags are named in the tests' documentation; a
  change in Chrome's switches fails the lane loudly.
- [Chromium's limit of peer connections] -> the page's census and the default cap of 256.
- [Depends on webcpp::asio's bridge and JSPI/Asyncify variants] -> increment 2 waits for the
  asio proposal; increment 1 needs only the executors.

## Migration Plan

There is nothing to migrate: no program depends on webcpp::webrtc. trystero's `backend` goes in
increment 3, a later change of trystero's; until then trystero keeps its own.

To start the implementation: write `reference/plan.md` and the reference code (tasks group 0),
get the owner's decisions of the Open Questions, then implement the increments in order. The
library is created as a repository `webcpporg/webrtc` by the owner, checked out as `libs/webrtc`.

Rollback: the proposal is a draft; withdrawing it deletes `openspec/changes/genesis/`.

## Open Questions

- libdatachannel's licence. MPL-2.0 is decided case by case by the owner, and no entry records
  that decision for libdatachannel. webcpp uses it unmodified, so MPL's file-level copyleft
  reaches none of webcpp's files; the decision to record is that MPL-2.0 is accepted for this
  dependency.
- The compile-time message without exceptions: the spec follows the rule's "declares nothing"
  (the compiler's "no member named 'peer_connection'"); a declared stub with a `static_assert`
  naming libdatachannel and exceptions would be clearer but departs from the rule. Does the
  owner want the stub?
- Whether a later version synthesises `negotiationneeded` natively.
- The exact names of the headers, the Jamfile targets and the library's repository layout,
  which the plan fixes.
- The precise form of webcpp::asio's browser bridge that increment 2 uses (its proposal is a
  draft).
- Whether libdatachannel is built from `third_party/` before or after the first release.
