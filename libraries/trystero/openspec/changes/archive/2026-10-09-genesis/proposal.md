## Why

A web program that wants peers to talk to each other without a server of its own needs a way for
them to find one another and a channel between them. Trystero (JavaScript) does both with public
Nostr relays for the meeting and WebRTC data channels for the conversation, and its rooms are used
by pages that have no back end. C++ had no such library: a native program or a C++ page compiled
to WebAssembly could not join a Trystero room, and a room joined from C++ could not see the peers
of the same room joined from JavaScript. webcpp::trystero gives C++20 programs Trystero's rooms,
on its wire, byte for byte, so that a C++ peer and a Trystero JS peer in one room are one room.

## What Changes

- **New library `webcpp::trystero`**, a port of Trystero 0.26.0 (MIT) and header-only C++20: a
  `room` that joins a room of an application over Nostr relays, reports the peers that join and
  leave, measures round trips and leaves with Trystero's farewell; an `action<T>`, a named channel
  of text, JSON (`boost::json::value`), binary or any of the three, with optional JSON metadata,
  of any size, in chunks with backpressure; the room's password, which seals the signaling and
  adds a challenge to the handshake; and every asynchronous operation as an Asio initiating
  function.
- The wire is Trystero's and never changes: topics, room tokens, sealed signals, Nostr events,
  action frames, envelopes, presence, default relays and their order, proven against Trystero 0.26.0
  itself by the `oracle` lane (the values its own functions compute) and the `interop` lane (C++
  peers and Trystero JS peers meeting in headless Chrome).
- A backend interface (`backend.hpp`) separates the rooms, which build on every target, from the
  platform: the browser backend (emscripten, over the page's WebSocket, RTCPeerConnection and
  Web Crypto through embind) and the native backend (libdatachannel, Boost.Beast, OpenSSL).
- Every allocation of a room goes through the `std::pmr::memory_resource` of its configuration;
  the library raises no exception of its own (an operation that can fail completes with an error
  code) and the rooms, the protocol and the browser backend compile without exceptions.
- A signaling that a peer or a relay cannot make a room hold without bound: negotiations,
  connections, replays, reflected offers, relay texts and unsealed signals are all bounded and
  tested.
- A memory backend for the tests, which runs whole rooms in one process, and a documentation page
  with a reference, a changelog and three examples.

## Capabilities

### New Capabilities

- `trystero`: webcpp::trystero 0.1.0, serverless WebRTC rooms for C++20, wire compatible with
  Trystero 0.26.0.

### Modified Capabilities

None.

## Impact

- **Kind:** a port of Trystero 0.26.0 (`"port-of"` in `meta/libraries.json`: JavaScript, MIT);
  `LICENSE-TRYSTERO.txt` is Trystero's notice, unchanged, and the library itself is under the
  Boost Software License.
- **Implementation:** https://github.com/webcpporg/trystero at `160f431`, the submodule
  `libs/trystero` of the superproject; `reference/source/` is that tree.
- **Dependencies:** Boost 1.92 (Asio, Container's pmr base class, Core, Hash2, JSON, System, and
  Beast for the native backend), libsecp256k1 0.8.0 (BIP-340 signatures of Nostr events),
  libdatachannel 0.24.6 and OpenSSL 3 (the native backend only), Emscripten 6.0.11 (the browser
  backend). Test data and oracle: Trystero 0.26.0 itself, pinned by `package-lock.json`, with Node
  and headless Chrome.
- **Status:** implemented and published, in release 0.1.0 (`reference/STATUS.md`).
- **What follows:** the change `shared-connection` (one connection shared by the rooms of an app,
  which the page lists as a difference and a roadmap item).
