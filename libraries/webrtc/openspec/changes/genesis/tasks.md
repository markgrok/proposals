# Tasks

Nothing is built. There is no plan and no reference code: group 0 writes them, and the proposal
is not accepted before it is done. Sections cited are those of `reference/spec.md`. A checked
task is done.

## 0. Before the proposal can be accepted

- [ ] 0.1 Write `reference/plan.md`: the implementation plan, one task per step with its files, interfaces, steps, code and expected output, in the three increments of section 10
- [ ] 0.2 Write the reference code: the interface of section 4 (types, `runtime`, `basic_peer_connection`, `basic_data_channel`, `error`) and a first native implementation over libdatachannel, as files under `reference/source/`, with the tests of section 8 it passes; then update `reference/STATUS.md`
- [ ] 0.3 Obtain the owner's decisions of section 11: libdatachannel's MPL-2.0 licence, and whether the no-exceptions message is the compiler's or a stub's
- [ ] 0.4 Check the draft against webcpp::asio's genesis (its bridge and the executors) and against trystero's genesis (its `backend`, section 3's table), and fix any difference
- [x] 0.5 The draft specification, with the owner's decisions of 2026-10-10 and the controller's rulings on the review (section 2), done 2026-10-10 (`reference/spec.md`)
- [x] 0.6 The review's answers: no zero-copy receive for now, and separate event operations with the policies of section 5.2 (section 10), done 2026-10-10

## 1. Increment 1: the interface, the `runtime` and the native implementation

- [ ] 1.1 The repository `webcpporg/webrtc` as a superproject submodule: `build.jam`, `meta/libraries.json`, `config.hpp` (includes `<webcpp/config.hpp>`, defines `WEBCPP_WEBRTC_NO_EXCEPTIONS`, the `UNSUPPORTED_*` macros, the `#error` under `__wasi__`), the configuration test built three times
- [ ] 1.2 The enumerations and the input and output value types of section 4 (`sdp_type`, the states, `ice_transport_policy`, `ice_server`, `configuration`, `session_description_view`, `ice_candidate_view`, `data_channel_init`, `basic_session_description`, `basic_ice_candidate`)
- [ ] 1.3 `options` and `runtime_options`, and the `error` category `webcpp.webrtc` with its ten enumerators (section 5.1)
- [ ] 1.4 The event machinery common to both backends: the coalescing of states, the bounded queues, one wait of a kind, the cancelled wait that consumes nothing (section 5.2), testable without a platform
- [ ] 1.5 The native `runtime`: thread pool size, `rtc::Cleanup()` on destruction, one at a time, the count of peer connections (section 5.3)
- [ ] 1.6 The native `basic_peer_connection`: `make`, descriptions, candidates and the end of candidates, states, `create_data_channel`, `async_accept_data_channel`, `close`, destruction with `resetCallbacks()` (sections 5.1 to 5.3)
- [ ] 1.7 The native `basic_data_channel`: open, close, receive in libdatachannel's pull mode, send, text, buffered amount, `max_message_size` and `max_buffered_amount` (section 5.4)
- [ ] 1.8 Native exceptions: every call into libdatachannel caught and turned into a value; the native headers declare nothing under `WEBCPP_WEBRTC_NO_EXCEPTIONS`; the headers-alone and `webcpp.compile-diagnostic` tests (section 6)
- [ ] 1.9 Native tests: two peers in one process, every case section 8 names, allocation through a counting resource, destruction under ThreadSanitizer, `run()` returning only once closed, the `runtime` stopping libdatachannel's threads before `main` returns
- [ ] 1.10 Measure whether a never-read native channel stalls the connection's other channels (section 5.4) and record the finding in the documentation
- [ ] 1.11 Documentation: the library's page, libdatachannel's justification, the table of differences (section 7), Doc Comments, the reference, examples
- [ ] 1.12 Lint, the lanes that apply to a native-only increment, and the review; increment 1 complete

## 2. Increment 2: the browser implementation and the interop lane

- [ ] 2.1 Wait for webcpp::asio's bridge and suspension variants (its genesis), and take their names
- [ ] 2.2 The emscripten `runtime`: the page's census of `RTCPeerConnection`s kept in JavaScript, `max_peer_connections`
- [ ] 2.3 The emscripten `basic_peer_connection`: descriptions through promises, candidates, states, channels, events entering through the bridge, the registration removed and the handlers nulled on destruction (sections 5.1 to 5.3)
- [ ] 2.4 The emscripten `basic_data_channel`: the byte-bounded receive queue and `receive_overflow`, `message_too_large` closing the channel, send, buffered amount (section 5.4)
- [ ] 2.5 `bind_address` and the other differences of section 7 under `WEBCPP_WEBRTC_UNSUPPORTED_ERROR` and `WEBCPP_WEBRTC_UNSUPPORTED_IGNORE`
- [ ] 2.6 Emscripten tests: every case of section 8 in one page, both suspension variants, exceptions on and off, allocation
- [ ] 2.7 The interop lane: a native peer and a headless-Chrome peer, signalled through a local relay, with Chrome's two flags and `bind_address` `127.0.0.1`, both suspension variants
- [ ] 2.8 Lint, all lanes, the review; increment 2 complete

## 3. Increment 3: trystero on webcpp::webrtc

- [ ] 3.1 A change of trystero's (its own proposal) that replaces `backend`, `native/peer_connection.hpp`, `native/data_channel.hpp` and `browser/backend.hpp` with this library, using the table of section 3
- [ ] 3.2 trystero keeps its policies: dropping text messages, dropping binaries larger than a room frame, its per-minute window of connections, and the merge of the two states
- [ ] 3.3 trystero's native and browser suites pass unchanged on the new library; its `backend` is gone; webcpp::webrtc is recorded as its dependency
