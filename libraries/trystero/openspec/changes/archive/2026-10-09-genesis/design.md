## Context

The trystero code was first written in an earlier private project, as a header-only C++20 port of
Trystero 0.26.0 with a CMake build and Boost.Test unit tests. Stage 4 of webcpp's plan
(`reference/plan.md`, written 2026-10-09) moved it into webcpp as `webcpporg/trystero`: renamed to
`webcpp::trystero` under `<webcpp/trystero/...>`, built by the superproject's b2 without CMake,
born with webcpp's allocation rule, tested natively and on Emscripten, documented on the shared
documentation toolchain with a MrDocs reference, and checked by two lanes of its own. The same
plan applied the owner's exceptions decision of 2026-10-08 to the superproject and made
Emscripten a tested target of its build and CI; those parts live in the superproject and are not
this library's requirements. Between 2026-10-09 and 2026-10-12 the library was hardened against
hostile peers and relays, built with MSVC 14.3 and 14.5, and given a changelog; the commit at which
this proposal reads it is `160f431`.

This document records, retroactively, the decisions the library is built on. What the owner asked
for next (one connection shared by the rooms of an app) is the change `shared-connection`.

## Goals / Non-Goals

**Goals:**

- Join a Trystero room from C++ and meet Trystero JS peers in it: the same wire, byte for byte,
  with the browser's WebRTC from WebAssembly and libdatachannel natively.
- Keep the room independent of the platform: the same room runs over the browser's APIs, over
  libdatachannel and over the memory relays of the tests.
- Raise no exception of its own, allocate only through the room's memory resource, and make every
  asynchronous operation usable with any Asio completion token.
- Bound everything a peer or a relay can make a room hold, since Trystero JS bounds none of it.
- Prove the wire against Trystero 0.26.0 itself, not against a reading of its source.

**Non-Goals:**

- Audio and video (`addStream`, `addTrack` and their renegotiation), request actions
  (`kind: 'request'`), `onReceive` and the progress callbacks, the application's own handshake
  (`onPeerHandshake`), passive rooms, one connection shared between the rooms of an app, and every
  strategy but Nostr (BitTorrent, MQTT, Firebase, Supabase, IPFS and the WebSocket relay). The page
  lists each with its reason.
- Building libsecp256k1, libdatachannel and OpenSSL: they are found where b2's command line or the
  superproject's `.local/` says, until webcpp builds them from `third_party/`.
- Running a program of the library under the build: every example needs the public relays or a
  browser, so each is linked and never run.

## Decisions

### D1. A port whose wire never changes

The wire is Trystero's: the topic prefixes `"Trystero@"` and `"Trystero:"`, the handshake's
`"__trystero_pw"`, every constant and message layout under `protocol/`, the default relays in
their order and the names embind exports (`trystero_dispatch`). A rename of the library never
replaces the word `trystero` by a pattern: it uses exact C++ ones (`namespace webcpp::trystero`,
`<webcpp/trystero/`, `WEBCPP_TRYSTERO_`), and a string literal changes only when it is not on the
wire (the error category `webcpp.trystero`, the examples' app ids). The `oracle` lane has Trystero's
own functions compute the protocol's values and fails when they differ from the committed ones; the
`interop` lane meets real Trystero JS peers.

### D2. Layers: protocol, backend interface, room, two backends

The protocol (`protocol/*.hpp`) is pure functions over bytes and text, with no I/O and no platform.
The backend interface (`backend.hpp`) is the WebSocket, peer connection, data channel, timer and
AES-GCM sealer a platform provides. The room (`room.hpp`, `detail/*.hpp`) is the Nostr strategy,
the signaling, the peer links, the action wire and the public API. The browser and native backends
implement the interface. The core (everything above the interface) builds on both targets and
never includes a backend, libdatachannel, OpenSSL or Emscripten: `meta/include-boundaries.json`
keeps it so and the lint fails an include that crosses it.

### D3. The backend is abstract classes, not template parameters

Each of a backend's operations is a call into JavaScript or a network stack, so a virtual call
costs nothing measurable, while the room stays free of templates over the platform and of
`#ifdef`, and the test backend and the browser backend are interchangeable at run time. Every
handler a backend invokes runs on the room's executor, after the call that caused it has
returned, never inside it, because the room calls these operations from inside handlers whose
state a completion run there would change under them.

### D4. The browser's own WebRTC, not a wrapper

libdatachannel's WebAssembly port has no media and hides the `RTCPeerConnection` it wraps. Binding
the browser's objects through `emscripten::val` is smaller, reports every error and leaves the
browser's whole WebRTC stack (capture, codecs, playout) within reach for media. No JavaScript
exception enters WebAssembly: every call that can throw goes through `browser/js.hpp`, whose
JavaScript runs it in `try` and returns a value or the exception as a value.

### D5. Text is a view, a copy is a decision

A parameter that only reads text is a `std::string_view`, bytes are a `std::span`, and a string
is used where something must own the text. Digests are fed the parts of their input, signals and
relay messages are read as views into the parsed document, a frame is written once inside its room
envelope and shared by every target, and a sentence of the log is composed only when there is a
sink. The asynchronous operations are the exception, for the reason the usage page gives.

### D6. Errors are values; the library raises nothing

An operation that can fail completes with a `boost::system::error_code` of the category
`webcpp.trystero`. Whether a program uses exceptions is its developer's choice on every target
(owner decision of 2026-10-08), and the library keeps compiling without them. The one stated
exception is the native backend's WebRTC (`native::data_channel`, `native::peer_connection`,
`native::backend`): libdatachannel reports errors by throwing, so those classes catch, sit behind
`#ifndef WEBCPP_TRYSTERO_NO_EXCEPTIONS` and do not exist without exceptions; the native timer,
sealer and WebSockets do.

### D7. Every allocation through a `std::pmr` resource

`room_config::memory` (default: the default resource) reaches every allocation of a room: its
state, handlers, JSON (through `json_resource`, a header-only adapter to Boost.Container's pmr
base, never its compiled global resources), the objects its backend makes for it, and the messages
an action's handler receives. The `handler` type holds every callable and `detail::post` every
post. What libdatachannel, OpenSSL and the browser's JavaScript allocate stays theirs, and the
page says so. A counting test replaces the global `operator new` and proves it natively and on
Emscripten, under AddressSanitizer.

### D8. Bounded signaling

Announcements are not sealed, so anyone who knows the app and room ids can announce any number of
peer ids, and anyone who reads the relays can send any signal in any peer's name. The room bounds
what that costs: a negotiation changes nothing before its signal opens with the room key; the
unopened signals are reported paced; replays and reflections of the room's own offers and answers
are refused by a cache of the last 1024; unconnected negotiations (32), connections per window
(64 per 57 s), a page's peer connections (256), a relay's text (1 MiB) and queue (4 MiB), and every
field of a signal are bounded. What remains is stated on the page, not hidden.

### D9. A memory backend for the tests, and two lanes of the library's own

`test/support/memory_backend.hpp` runs whole rooms in one process: relays that forward ephemeral
events, peer connections that pair through their descriptions, data channels that deliver in order
and a sealer that opens only what was sealed under the same key. No test reaches beyond the
loopback. The `oracle` and `interop` lanes are the library's own, with Node, Trystero 0.26.0 and
headless Chrome, on native and Emscripten.

### D10. Optional native dependencies

libdatachannel (MPL 2.0) and OpenSSL (Apache 2.0) are needed by the native backend only:
`/webcpp/trystero//trystero` depends on neither and `/webcpp/trystero//native` adds them. Natively
a library found nowhere leaves trystero out of the build with one message; the CI requires them.

### D11. Licensing

The library is under the Boost Software License. It ports Trystero's protocol and was written with
Trystero's source open, so Trystero's MIT notice comes with it (`LICENSE-TRYSTERO.txt`, unchanged)
and the README names what derives from it.

## Risks / Trade-offs

- [Trystero changes its wire in a later version] -> the version is pinned (0.26.0) by the lock
  file, and the `oracle` lane fails when Trystero's own values differ from the committed ones.
- [libdatachannel must be built with the program's standard library] -> documented; the native
  tests run with the compiler that built it, and the container lanes cover libstdc++ for the core.
- [A flood still delays new peers] -> a flooder that keeps sending new ids takes each place of
  the bounded negotiations as it frees; joined peers, memory and connections stay safe. Stated
  in the page; Trystero JS bounds none of it.
- [A watcher without the key can keep two peers apart] -> only a wire that seals the offer id and
  both peer ids closes it, which Trystero would have to share (reported upstream).
- [Each room makes its own connections] -> the difference the change `shared-connection` removes.

## Migration Plan

None: a new library.

## Open Questions

None for 0.1.0. What follows (one connection per app, media, request actions, other strategies) is
the page's roadmap and the change `shared-connection`.
