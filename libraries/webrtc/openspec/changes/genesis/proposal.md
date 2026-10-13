## Why

A web program in C++ that talks peer to peer needs peer connections and data channels, and today
it has two bad choices: write the program twice, once against a native WebRTC stack and once
against the browser's JavaScript objects, or hide both behind a virtual interface of its own.
trystero, webcpp's serverless WebRTC rooms, took the second road: its `backend` interface, with
`native/peer_connection.hpp`, `native/data_channel.hpp` and `browser/backend.hpp`, is a callback
interface with one implementation per target, null on refusal, and it keeps WebRTC details
(the browser's cap of peer connections, the merge of two state machines) inside trystero where no
other program can use them.

webcpp::webrtc is that interface done as a library: the W3C WebRTC API's shape and names
(`RTCPeerConnection`, `RTCDataChannel`), whose operations follow Asio's model (completion
tokens, handlers on webcpp::asio's executors, errors as values), so that a program is written
once and runs natively (libdatachannel) and in a browser (Emscripten, driving the browser's own
objects). Media (tracks, codecs) is out of scope: data channels only, what trystero needs.

## What Changes

- **New library `webcpp::webrtc`**, webcpp's own design, header-only C++20, namespace
  `webcpp::webrtc`, for two targets: native and Emscripten (both suspension variants, JSPI and
  Asyncify). wasip2 and wasip3 are not targets: there is no WebRTC stack for WASI, and the
  library's convenience header stops with `#error` under `__wasi__`, naming the reason.
- The interface of the specification's section 4: `basic_peer_connection` and
  `basic_data_channel` templated on an executor and an allocator, value types for inputs
  (`configuration`, `ice_server`, `session_description_view`, `ice_candidate_view`,
  `data_channel_init`) and owned outputs (`basic_session_description`, `basic_ice_candidate`),
  the five state and policy enumerations, `options` (webcpp's own bounds), and a `runtime` that
  owns what the platform holds for the whole process.
- Operations are `async_*` members with any completion token and terminal cancellation; events
  (candidates, state changes, incoming channels, open, close, buffered amount low) are pending
  operations the caller re-arms, as Asio's `async_accept`, with a stated policy for an event
  that arrives while no wait is pending.
- Errors are values: `webcpp::webrtc::error`, an `error_category` named `webcpp.webrtc`, ten
  enumerators with fixed values, plus Asio's own codes with their usual meaning.
- Back-pressure and bounds: `max_message_size`, `max_buffered_amount`, `max_received_bytes`,
  `max_queued_candidates`, `max_queued_channels` and `max_peer_connections`, each failing as a
  value.
- The native backend wraps libdatachannel (unmodified, a dependency, justified in the library's
  documentation) and is available only where exceptions are on; the emscripten backend works
  with exceptions on and off.
- A native peer and a headless-Chrome peer connect to each other in the tests.
- Increment 3 moves trystero onto this library and removes trystero's `backend`; that is a
  change of trystero's, not of this proposal.

## Capabilities

### New Capabilities

- `webrtc`: webcpp::webrtc, the W3C WebRTC data-channel API in Asio's model for native and
  Emscripten, with its types, operations, event policies, error codes, bounds, lifetime rules,
  runtime, exception and allocation guarantees, per-target differences, tests and documentation.

### Modified Capabilities

None.

## Impact

- **Kind:** a library of webcpp's own (`port_of: null`): the interface and its semantics are
  webcpp's design; libdatachannel and the browser are implementations behind it (design.md, D1).
  Repository `webcpporg/webrtc`, to be created by the owner, checked out as the submodule
  `libs/webrtc`, BSL-1.0, "Copyright (c) 2026 WebCpp.org".
- **Implementation:** not started. There is no plan and no reference code: `reference/` holds
  the draft specification (`spec.md`) and `reference/STATUS.md` ("reference code: none yet;
  required before the proposal is accepted"). The first tasks are to write the plan and the
  reference code (tasks.md, group 0). trystero's `backend` and its two implementations are the
  nearest existing code and the precursor of the interface, but they live in trystero's
  reference, not here, and they are not this library's reference code.
- **Dependencies:** webcpp::asio (the executors, and on Emscripten its bridge, which only posts
  to the io_context); Boost.Asio, Boost.System (`result`, `error_code`) and Boost's other
  headers; webcpp::config (`<webcpp/config.hpp>`); libdatachannel 0.24.6 on native, unmodified,
  MPL-2.0 (the licence awaits the owner, design.md Open Questions), found today through
  `-sLIBDATACHANNEL_ROOT` or the superproject's `.local/libdatachannel`, pinned later when built
  from `third_party/`; the browser's `RTCPeerConnection` and `RTCDataChannel` on Emscripten.
  It knows nothing of xactor.
- **Status:** draft; no release; no implementation; not decided.
- **Other proposals:** depends on webcpp::asio's genesis (`libraries/asio`) for the executors
  and the browser bridge. trystero (`libraries/trystero`) is its first user: increment 3 is a
  later change of trystero, which replaces its `backend`, keeps its own policies (dropping text
  messages and oversized frames, its per-minute window of connections) and merges the two
  states itself. Native encryption and randomness stay in webcpp::crypto, WebSockets in
  webcpp::beast.
