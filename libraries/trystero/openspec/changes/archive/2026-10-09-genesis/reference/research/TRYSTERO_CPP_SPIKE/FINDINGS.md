# trystero-cpp: feasibility findings

Date: 2026-10-05. Status: spike. Everything under `spike/probes/` is throwaway code written to answer one question each; none of it is a starting point for the library.

## The question

Can Trystero be ported to C++ as a library in `namespace trystero`, in a shape that could one day be proposed to Boost, that compiles with Emscripten to run in the browser, uses libdatachannel natively, and keeps everything WebRTC offers, including audio and video?

## The answer

Yes, with two adjustments to the premise:

1. **In the browser the library should not use libdatachannel or datachannel-wasm.** datachannel-wasm has no media at all, and its `RTCPeerConnection` is hidden behind a private integer id. C++ can drive the browser's own `RTCPeerConnection` directly through `emscripten::val` and C++20 coroutines. That was measured, and it gives the browser's whole WebRTC stack (capture, codecs, playout, simulcast, stats) at no cost. libdatachannel is the native backend only.
2. **"Everything WebRTC offers" means different things on the two targets.**
   - In the browser it is literally everything, because the browser is the media engine.
   - Natively, libdatachannel is a transport. It moves encrypted RTP, packetizes and depacketizes H264, H265, VP8, VP9, AV1, Opus and PCM, and handles RTCP (NACK, PLI, REMB, SR). It does not capture, encode, decode or render.
   - So the library's media API has to be honest about that difference rather than hide it.

For Boost, the hard part is not the code. It is a mandatory MPL-2.0 dependency (libdatachannel and libjuice) and the MIT origin of Trystero's code. Both have workable answers (section 6).

## What was measured

Every row was run on this machine:
- macOS on arm64
- Emscripten 6.0.11
- libdatachannel 0.24.6 (`773e5b3`)
- Boost 1.92 headers
- chrome-headless-shell from the Playwright cache, with fake capture devices

| #   | Claim                                                                      | Evidence                                                                                                                                                                                                                                                                                                                                                          |
| --- | -------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1   | C++20 compiled with Emscripten can drive the browser's `RTCPeerConnection` | `spike/probes/media-interop/browser/main.cpp` does all of this from C++ through `emscripten::val`, with `co_await` on the browser's promises: creates the peer connection, `getUserMedia` (audio and video), `addTrack`, `createDataChannel`, `createOffer`, `setLocalDescription`. The wasm is 31 KB at `-O1`                                                          |
| 2   | A browser peer and a native libdatachannel peer interoperate               | Connection `connected`; DTLS `connected`, SRTP `SRTP_AES128_CM_HMAC_SHA1_80`; data channel text both ways                                                                                                                                                                                                                                                         |
| 3   | Browser to native media works                                              | In one run the browser sent 391 Opus packets and 171 VP8 packets. Native received 351 RTP packets on `opus pt 111` and 165 on `VP8 pt 96`; the counters were read a moment earlier than the browser's. The first VP8 payload carries the key frame start code `9d 01 2a`                                                                                          |
| 4   | Native to browser media works                                              | Native re-sent every received packet under its own SSRC, announced in its answer. The browser's `inbound-rtp` showed 176 VP8 packets and 135 frames decoded at 480x360. It also showed 392 Opus packets, with 0 samples because headless Chrome has no audio output to play them on                                                                               |
| 5   | Boost.Asio runs in the browser's execution model                           | `spike/probes/asio-emscripten/main.cpp` uses `post`, `steady_timer` and an `awaitable` coroutine, with `-DBOOST_ASIO_DISABLE_THREADS`. `io_context::poll()` is driven every 10 ms by `emscripten_set_interval`, never `run()`. Trystero's announce timers (0, 233, 766, 2099 ms) fired at most 9 ms late. `asio_poll_strand.cpp` adds a strand. The wasm is 82 to 99 KB |
| 6   | The Nostr crypto builds for wasm                                           | `spike/probes/crypto/crypto_spike.cpp` builds libsecp256k1 with `schnorrsig` under `emcmake`, plus Boost.Hash2 SHA-1 and SHA-256. BIP-340 sign and verify passes, and the SHA test vectors are correct. The wasm is 152 KB (60 KB gzipped) with `ECMULT_WINDOW_SIZE=4` and `ECMULT_GEN_KB=2`, and 1.30 MB with the default tables                                       |
| 7   | AES-GCM in the browser is WebCrypto from C++                               | `spike/probes/crypto/webcrypto_spike.cpp` round-trips AES-GCM with Trystero's 16-byte IV through `crypto.subtle` and `co_await`. The wasm is 23.6 KB                                                                                                                                                                                                                    |

## Pitfalls the probes hit

Each pitfall below cost a debugging round. Each one is a requirement for the native backend, not a curiosity.

1. **libdatachannel keeps only `weak_ptr`s.**
   - **Tracks.** A `Track` handed to `onTrack` that the application does not retain is destroyed when the callback returns, and the answer then rejects that media section with port 0 (`src/impl/peerconnection.cpp`, "track is destroyed"). The browser reports nothing and simply stops its transceivers.
   - **WebSockets.** The same happens to a `WebSocket` from `WebSocketServer::onClient`.
   - **Requirement:** the backend owns every object the library hands it.
2. **The default answer accepts every codec offered.**
   - **What it causes.** Chrome wrapped VP8 in RED (pt 118, RFC 2198) with ULPFEC, and probed bandwidth with RTX padding (pt 97). A receiver that only depacketizes VP8 sees no VP8 at all.
   - **Requirement.** The native backend must prune each remote media description in `onTrack` to the codecs it can handle, then `setDescription` before the answer is generated.
   - **Measured result.** After pruning, the answer carried only `opus pt 111` and `VP8 pt 96`, and the packets arrived as plain VP8.
3. **`Description::Media::reciprocate()` copies the offerer's `a=msid`.**
   - **What it causes.** Announcing our own SSRC with `addSSRC(..., msid, ...)` then puts two msids in one section, and Chrome rejects the whole answer ("Two different track IDs in msid attribute in one m= section").
   - **Requirement.** Strip the echoed `msid:` attributes first.
4. **`Description::Media::rtpMap(pt)` throws when the payload type is absent**, and removing a codec also removes its RTX entry. Check `hasPayloadType` first.
5. **libdatachannel swallows exceptions thrown from callbacks.** It logs a WARN and carries on. The exception from pitfall 4 silently rejected the video section. **Requirement:** the backend must catch inside every callback and turn the failure into an error the library reports.
6. **`setRemoteDescription` failures in the browser are silent.** They are rejected promises nobody awaits. The browser backend must await every promise it creates.
7. **`Description::Media::rtpMap` returns a pointer into the description.** Calling it on the temporary returned by `track->description()` leaves the pointer dangling, and the probe printed codec names as empty strings. **Requirement:** hold the description in a variable.

## libdatachannel and datachannel-wasm

This section is condensed from a source read of both libraries; file and line references are in the spike's working notes.

**datachannel-wasm.** It implements a subset of libdatachannel's API over the browser:
- No `Track`, no media and no explicit `setLocalDescription`, `createOffer` or `createAnswer`; offers and answers are always automatic.
- No `negotiated` or `id` data channels, and no error reporting to C++.
- Text messages are truncated at an embedded NUL.
- `isClosed()` stays false after `close()`.
- It needs `window`.

So it cannot carry Trystero's media, perfect negotiation or error paths. A direct `emscripten::val` binding is smaller than the 1,400 lines of glue it would replace, and it exposes everything.

**libdatachannel natively** covers:
- data channels with all options;
- media transport with packetizers and RTCP handlers;
- explicit negotiation and local rollback;
- wss through OpenSSL.

Its limits:
- **Negotiation.** An incoming offer while it holds a local offer triggers an implicit rollback, so it always behaves as the polite peer, and the impolite side must drop offers itself.
- **No `onNegotiationNeeded`.** `addTrack` does not offer on its own.
- **Threads.** Callbacks run on its thread pool, serialized per connection, under a recursive mutex. The backend must only copy and post to the library's executor from them, and never block.

## How the library would look

### Principles

- **Boost style.** Header-mostly C++20, Boost.Asio as the execution model, completion tokens on every asynchronous operation (callbacks, `use_awaitable`, futures), and `error_code` overloads.
- **One portable core, two targets.** Everything that is the Trystero protocol is pure code, tested natively and identical in the browser. Everything that touches a network or a media engine sits behind a concept with one model per target.
- **Wire compatible with Trystero 0.26.** A C++ peer must be able to join a room that JavaScript peers are in. That is the acceptance test of the whole port.

### Layers

| Layer                 | Responsibility                                                                                                                                                                                                                                                                                  | Browser model                                                        | Native model                                                                                       |
| --------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------- |
| `trystero::protocol`  | Topics (`sha1("Trystero@appId@roomId")`, self topic), the announce schedule, offer and answer sealing, the room envelope, action wire format v2 (16 KiB frames, chunking, reassembly), the handshake (`@_hsready`), arbitration (the lower `selfId` offers), perfect negotiation, passive peers | Same code                                                            | Same code                                                                                          |
| `trystero::crypto`    | SHA-1 and SHA-256 (Boost.Hash2 everywhere), AES-GCM with a 16-byte IV, random bytes, BIP-340 for Nostr                                                                                                                                                                                          | WebCrypto via `emscripten::val`, which is asynchronous; libsecp256k1 | OpenSSL or a portable AES-GCM; libsecp256k1                                                        |
| `trystero::rtc`       | A peer connection concept: description, candidates, data channels, tracks, stats, the per-connection strand                                                                                                                                                                                     | `RTCPeerConnection` via `emscripten::val`                            | libdatachannel, with pitfalls 1 to 5 handled once, here                                            |
| `trystero::signaling` | One strategy per relay network: Nostr first, then MQTT and BitTorrent trackers; each is a WebSocket protocol                                                                                                                                                                                    | The browser `WebSocket` via `emscripten::val`                        | Boost.Beast WebSocket over Asio SSL, which is Boost only; libdatachannel's WebSocket is not needed |
| `trystero::room`      | The public API: join, actions, peers, streams and tracks, leave                                                                                                                                                                                                                                 | Same code                                                            | Same code                                                                                          |

Because WebCrypto is asynchronous, the crypto concept is asynchronous on both targets. Natively it completes through `post`.

### Media, honestly typed

The data plane is uniform across targets:
- actions;
- binary payloads;
- metadata;
- progress callbacks.

The media plane is not uniform. The room API takes and gives a target-specific track type, and a portable program writes the media part twice, under `if constexpr` or the preprocessor.

- `trystero::browser::media_track`: a `MediaStreamTrack` handle (`emscripten::val`). Capture is `getUserMedia`; rendering is a `<video>` element; the codec choice is the browser's.
- `trystero::native::outgoing_track` and `incoming_track`: an RTP track with a negotiated codec list, `send_frame(encoded, timestamp)`, `on_frame`, keyframe requests and bitrate hints. Encoders, decoders and capture come from the application: libopus, libvpx or OpenH264, and the OS capture APIs.

### A sketch of the API

This illustrates the shape and is not a design.

```cpp
#include <trystero/trystero.hpp>
#include <trystero/signaling/nostr.hpp>

namespace asio = boost::asio;

asio::awaitable<void> chat(trystero::room& room) {
    auto chat = room.make_action<std::string>("chat");
    room.on_peer_join([](trystero::peer_id peer) { /* ... */ });

    co_await chat.send("hello", trystero::all_peers, asio::use_awaitable);
    for (;;) {
        auto [text, from] = co_await chat.receive(asio::use_awaitable);
    }
}

int main() {
    asio::io_context io;
    trystero::room_config config{
        .app_id = "webcpp-demo",
        .password = "optional, seals offers and answers",
        .rtc = {.ice_servers = {"stun:stun.l.google.com:19302"}},
    };
    auto room = trystero::join_room<trystero::signaling::nostr>(io.get_executor(), config, "lobby");
    asio::co_spawn(io, chat(room), asio::detached);
    trystero::run(io);  // native: io.run(); browser: poll() from the event loop, returns at once
}
```

### Proposed layout

```
include/trystero/
    trystero.hpp
    room.hpp  action.hpp  peer_id.hpp  config.hpp  error.hpp  run.hpp
    protocol/   topic.hpp  announce.hpp  envelope.hpp  chunking.hpp  handshake.hpp  arbitration.hpp  negotiation.hpp
    crypto/     concept.hpp  hash2.hpp  webcrypto.hpp  openssl.hpp  secp256k1.hpp
    rtc/        concept.hpp  browser/peer_connection.hpp  native/peer_connection.hpp
    signaling/  nostr.hpp  mqtt.hpp  torrent.hpp  websocket/browser.hpp  websocket/beast.hpp
    detail/
test/       unit tests of the protocol (native), interop tests against Trystero JS (headless Chrome)
example/    chat, file transfer, video call (browser), video relay (native)
doc/
build.jam   CMakeLists.txt
```

## The Boost question

These facts come from the contributor guide; the sources are in the research notes.

- **License.** Boost wants BSL-1.0 and forbids a license that requires notices or source availability for binary use.
- **Dependencies.** A library depends only on Boost and the standard library unless the mailing list agrees. Accepted precedents:
  - optional zlib and OpenSSL (Asio, Beast);
  - Redis and MySQL require OpenSSL unconditionally;
  - ICU (Locale).
- **No precedent for copyleft.** No Boost library has a mandatory copyleft dependency, and libdatachannel and libjuice are MPL-2.0.
- **Build.** Both `build.jam` and `CMakeLists.txt` are required. b2 ships an Emscripten toolset, and Boost.Config already runs Emscripten in CI.
- **Process.** Endorsements, a review manager, then a review of about ten days. The guide warns that getting in can take years.

What this means for the design:

1. **The browser model has no third-party dependency at all.** That is already true of the measured design: `emscripten::val`, WebCrypto and the browser `WebSocket`.
2. **The native WebRTC model is an optional adapter.** It is libdatachannel behind the `rtc` concept, the way Asio treats OpenSSL. The library proper stays Boost only. Nostr's libsecp256k1 (MIT) has the same status: the Nostr strategy is optional, and MQTT and the trackers need no secp256k1 at all.
3. **The code is written from a protocol specification, not translated from Trystero's source.** Trystero is MIT, and MIT requires its notice in substantial portions, binaries included, which BSL does not. There are two clean routes:
   - write a specification of the wire protocol (the developer guide is most of it) and implement the C++ from that;
   - or ask Trystero's eleven contributors to dual-license under BSL-1.0, as fast_float did for Boost.Charconv.
   
   This is an engineering reading, not legal advice.

Being Boost-ready from day one is achievable: BSL, b2 and CMake, Boost-only core, documentation, and CI on two compilers plus Emscripten. Acceptance is a social process on its own timeline.

## Open questions, in the order to answer them

1. **Interop with JavaScript Trystero.**
   - The probes proved C++ against a native peer, not C++ against Trystero's wire format.
   - The next experiment: a C++ peer joins a Nostr room with a JS Trystero peer and exchanges an action.
   - Run it first in the browser model, then natively.
2. **Driving Asio without a 10 ms poll.**
   - A custom executor that schedules a microtask when work is posted would wake exactly when needed.
   - Unmeasured.
3. **Native AES-GCM.**
   - The options are OpenSSL (the Asio precedent) or a small portable implementation.
   - The choice affects the optional-dependency story.
4. **Perfect negotiation on libdatachannel.**
   - Trystero renegotiates when tracks are added.
   - The native backend must emulate `onnegotiationneeded` and the impolite peer's offer dropping.
5. **Size of a real browser build.**
   - The pieces measured 23 to 152 KB each.
   - The total is unmeasured.

## Reproducing the probes

The toolchain and dependencies stay out of git. Set them up as follows:
- `third_party/libdatachannel` at `773e5b3`: built with CMake into `third_party/libdatachannel/build`, with media enabled.
- `third_party/datachannel-wasm` at `f9200eb`: read, not used.
- `.tools/emsdk`: Emscripten 6.0.11. Activate it with `source .tools/emsdk/emsdk_env.sh`.

| Probe | Build and run |
|---|---|
| `spike/probes/media-interop` | Native: `cmake -S native -B native/build && cmake --build native/build && native/build/peer`. Browser: `em++ -std=c++20 -O1 main.cpp -lembind --pre-js helpers.js -sENVIRONMENT=web -o probe.js` in `browser/`, served on `127.0.0.1:8771`. Start Chrome with `--remote-debugging-port=9336 --use-fake-ui-for-media-stream --use-fake-device-for-media-stream`. Run `node stats.mjs` for the browser's stats and the native counters |
| `spike/probes/asio-emscripten` | `em++ -std=c++20 -O2 -I<boost> -DBOOST_ASIO_DISABLE_THREADS -fexceptions main.cpp -sENVIRONMENT=node -o probe.js && node probe.js` |
| `spike/probes/crypto` | libsecp256k1 through `emcmake cmake -DSECP256K1_ENABLE_MODULE_SCHNORRSIG=ON -DSECP256K1_ECMULT_WINDOW_SIZE=4 -DSECP256K1_ECMULT_GEN_KB=2`, then `em++` with Boost.Hash2's headers. `webcrypto_spike.cpp` needs `-lembind` and runs under node, whose `crypto.subtle` matches the browser's |

## Addendum, 2026-10-05: a C++ peer in a Trystero JS room

The question: can a room opened by the C++ port and a room opened by Trystero JS 0.26 talk to each other?

The answer is yes. `spike/probes/js-interop/peer.cpp` (throwaway, about 600 lines) is a C++ peer compiled with Emscripten. It implements, over `emscripten::val`:
- **Discovery:** Trystero's Nostr discovery, with topics, kinds, the relay derivation and BIP-340 signed events through libsecp256k1.
- **The SDP exchange:** offer and answer, with candidates trickled behind them, sealed with AES-GCM through WebCrypto.
- **Connection:** the browser's `RTCPeerConnection`.
- **The room:** the room envelope, presence, the `@_hsready` handshake and inline actions.

`run.mjs` opens it in headless Chrome beside a page running Trystero JS from esm.sh, in a fresh room on the public relays.

Results:

| Run | Who offered | Result |
|---|---|---|
| C++ id `000...` | C++ | pass in 1.6 s: both peers joined; text and JSON actions crossed both ways and each side's echo came back |
| C++ id `zzz...` | Trystero JS, after the C++ peer's counter-announcement | pass in 1.1 s |
| random ids, three runs | C++ (by chance each time) | pass, about 1 s each |

The C++ peer derived the same five relays Trystero JS chose for the app id (`testr.nymble.world`, `nostr.stakey.net`, `relay.grigic.org`, `relay.nostr.dev.br`, `bendernostur.duckdns.org:8443`). That confirms the shuffle and its `sin` seed match.

One relay rate-limited the probe, because the probe publishes every candidate on every relay. Trystero answers only on the relay that delivered the message, and the library must do the same and respect the backoff.
