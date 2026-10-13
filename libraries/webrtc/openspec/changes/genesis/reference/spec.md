---
title: "webcpp::webrtc: the W3C WebRTC API in Asio's model, native and in the browser"
tags: [webcpp, webrtc, asio, emscripten, spec]
status: draft
created: 2026-10-10
---

# webcpp::webrtc: specification (draft)

## 1. Purpose

One interface for peer connections and data channels, shaped as the W3C WebRTC API, whose
operations follow Asio's model (completion tokens, handlers on webcpp::asio's executors), so
that a program, and trystero first, is written once and runs natively and in a browser.
Natively it wraps libdatachannel; compiled with Emscripten it drives the browser's
`RTCPeerConnection` and `RTCDataChannel`. Media (tracks, codecs) is out of scope: data
channels only, what trystero needs.

## 2. Decisions

### 2.1 The owner's (2026-10-10)

1. A library of its own, on webcpp::asio.
2. The W3C API's shape and names, in Asio's model, with completion tokens on webcpp::asio's
   executors.
3. Implemented by target: libdatachannel natively, wrapped and justified in its
   documentation (section 9); the browser's objects on Emscripten.
4. A native peer and a browser peer connect to each other in the tests.

### 2.2 The controller's rulings on the review (2026-10-10)

1. **Targets.** webcpp::webrtc's targets are native and emscripten. wasip2 and wasip3 are
   not targets: there is no WebRTC stack for WASI. libdatachannel needs threads, which
   neither WASI target gives, and a UDP socket layer and a DTLS stack that it has no WASI
   port of; Boost.Asio itself does not build for wasm32-wasip2 or wasm32-wasip3 with
   wasi-sdk 34 (AGENTS.md:1443-1446). The library's build declares native and emscripten
   only, and its convenience header stops with `#error` under `__wasi__`, naming this
   reason. A later WASI target is a change of its own.
2. **Exceptions.** libdatachannel reports errors by throwing, so the native backend calls
   it only where exceptions are on. Without them (`WEBCPP_WEBRTC_NO_EXCEPTIONS`) the
   native backend is not available, and a program learns it at compile time, by AGENTS.md
   chapter 6's rule for a header that wraps a throwing dependency: what follows the
   native headers' `config.hpp` sits inside `#ifndef WEBCPP_WEBRTC_NO_EXCEPTIONS`, with a
   comment that names libdatachannel, so a native program built without exceptions that
   names `webcpp::webrtc::peer_connection` fails on that name, and one that only includes
   the header compiles. The documentation names this on the library's page and in
   `config.hpp`'s Doc Comment. The emscripten backend works with exceptions on and off.
   trystero's native backend already requires exceptions (`native/peer_connection.hpp:12-15`,
   `native/data_channel.hpp:12-14`, `build.jam:91-95`).

### 2.3 How it meets xactor

AGENTS.md chapter 6 builds asynchronous ports on xactor (AGENTS.md:674-678), and the
owner's entry of 2026-10-10 puts webcpp::webrtc in Asio's model (WEBCPP_DESIGN.md:840).
They meet at the executor: an xactor actor that needs a peer holds a `peer_connection`
made on the executor its driver runs on, starts operations from its turns, and turns each
completion into a message to itself. webcpp::webrtc knows nothing of xactor.

## 3. What exists today, and where each part goes

trystero's `backend` interface (`backend.hpp`), with `native/peer_connection.hpp`,
`native/data_channel.hpp` and `browser/backend.hpp`, is this library's first draft: a
virtual interface with handler callbacks, one implementation per target. webcpp::webrtc
replaces it with value types and asynchronous operations; trystero's `backend` then goes
(increment 3). Every part of it that touches WebRTC has a home:

| trystero today                                                                                                   | webcpp::webrtc                                                                                                                                                              |
| ---------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `factory::make_peer_connection`, null on refusal (`backend.hpp:350-354`)                                         | `basic_peer_connection::make`, a `result` (section 4)                                                                                                                       |
| `peer_connection::set_local_description`, completing with the type and SDP (`backend.hpp:251-252`)               | `async_set_local_description`, completing with a `session_description`; `local_description()`                                                                               |
| `set_remote_description`, `add_ice_candidate`, `has_remote_description`                                          | the same operations; the candidate as a value, not JSON                                                                                                                     |
| `handlers::ice_candidate`, `data_channel`, `state`                                                               | `async_wait_ice_candidate`, `async_accept_data_channel`, `async_wait_state_change` and `async_wait_ice_connection_state_change`                                             |
| the browser's merge of `connectionState` and `iceConnectionState` (`browser/backend.hpp:647-671`)                | both states, separately; trystero merges them itself                                                                                                                        |
| `create_data_channel`, null on refusal (`backend.hpp:239-242`)                                                   | `create_data_channel`, a `result`                                                                                                                                           |
| `data_channel::is_open` (`backend.hpp:171`)                                                                      | `ready_state()`                                                                                                                                                             |
| `data_channel::handlers::open`, `message`, `close`, `buffered_amount_low`                                        | `async_wait_open`, `async_receive`, `async_wait_close`, `async_wait_buffered_amount_low`                                                                                    |
| `data_channel::send`, synchronous, dropped when not open (`backend.hpp:178`)                                     | `send`, synchronous, and `async_send`; both fail as values when not open                                                                                                    |
| `buffered_amount`, `low_threshold`                                                                               | `buffered_amount()`, `set_buffered_amount_low_threshold`                                                                                                                    |
| `backend_config::bind_address` (`native/backend.hpp:117`)                                                        | `configuration::bind_address`, native only (section 7)                                                                                                                      |
| the page's cap of 256 `RTCPeerConnection`s (`browser/backend.hpp:914-960`)                                       | `runtime_options::max_peer_connections` (section 4)                                                                                                                         |
| `native_handle()` (`native/peer_connection.hpp:256`, `browser/backend.hpp:642`)                                  | none: nothing of `rtc::` or `emscripten::val` appears in a public header, and media, which the handle was kept for, is out of scope; media will get an interface of its own |
| `backend_config::max_peer_connections_per_minute` (`native/backend.hpp:126`)                                     | stays trystero's: a policy of its rooms against floods, not a limit of a platform                                                                                           |
| dropping text messages and oversized binaries (`native/data_channel.hpp:164-169`, `browser/backend.hpp:403-407`) | stays trystero's: webcpp::webrtc delivers text and fails an oversized message (section 5.4); trystero ignores text and drops what is larger than a room frame, as today     |

The sealer, `random`, the timers, the clocks and the WebSockets go to webcpp::crypto,
webcpp::asio and webcpp::beast.

## 4. Interface

```cpp
namespace webcpp::webrtc {

enum class sdp_type { offer, answer, pranswer, rollback };       // RTCSdpType
enum class peer_connection_state { new_, connecting, connected, disconnected, failed, closed };
enum class ice_connection_state { new_, checking, connected, completed, disconnected, failed, closed };
enum class data_channel_state { connecting, open, closing, closed };  // RTCDataChannelState
enum class ice_transport_policy { all, relay };

// Inputs, read during the call that takes them: views, as RTCIceServer and RTCConfiguration.
struct ice_server {
    std::span<std::string_view const> urls;
    std::string_view username;
    std::string_view credential;
};
struct configuration {
    std::span<ice_server const> ice_servers;
    ice_transport_policy policy = ice_transport_policy::all;
    std::string_view bind_address;   // native only (section 7); empty: every interface but the loopback
};
struct session_description_view {
    sdp_type type;
    std::string_view sdp;
};
struct ice_candidate_view {          // RTCIceCandidateInit; an empty candidate ends the remote gathering
    std::string_view candidate;
    std::optional<std::string_view> sdp_mid;
    std::optional<std::uint16_t> sdp_m_line_index;
    std::string_view username_fragment;
};
struct data_channel_init {           // RTCDataChannelInit
    bool ordered = true;
    std::optional<std::uint16_t> max_packet_life_time;
    std::optional<std::uint16_t> max_retransmits;
    std::string_view protocol;
    bool negotiated = false;
    std::optional<std::uint16_t> id;
};

// Outputs, owned, their text in the object's allocator.
template <class Allocator>
struct basic_session_description {
    sdp_type type;
    std::basic_string<char, std::char_traits<char>, rebind_alloc<Allocator, char>> sdp;
    session_description_view view() const;
};
template <class Allocator>
struct basic_ice_candidate {
    std::basic_string<char, std::char_traits<char>, rebind_alloc<Allocator, char>> candidate;
    std::basic_string<char, std::char_traits<char>, rebind_alloc<Allocator, char>> sdp_mid;
    std::optional<std::uint16_t> sdp_m_line_index;
    std::basic_string<char, std::char_traits<char>, rebind_alloc<Allocator, char>> username_fragment;
    ice_candidate_view view() const;
};

// webcpp's own limits, past which an object fails as a value (section 5).
struct options {
    std::size_t max_queued_candidates = 64;
    std::size_t max_queued_channels = 16;
    std::size_t max_message_size = 256 * 1024;
    std::size_t max_buffered_amount = 16 * 1024 * 1024;
    std::size_t max_received_bytes = 16 * 1024 * 1024;   // emscripten only (section 5.4)
};

// What the platform holds for the whole process or page (section 5.3).
struct runtime_options {
    std::size_t max_peer_connections = 256;
    unsigned thread_pool_size = 0;   // native: libdatachannel's threads, 0 for its default
};
class runtime {
public:
    explicit runtime(runtime_options const& = {});
    ~runtime();                      // native: waits for rtc::Cleanup()
    runtime(runtime const&) = delete;
    runtime& operator=(runtime const&) = delete;
};

template <class Executor = boost::asio::any_io_executor, class Allocator = std::allocator<std::byte>>
class basic_data_channel;

template <class Executor = boost::asio::any_io_executor, class Allocator = std::allocator<std::byte>>
class basic_peer_connection {
public:
    using executor_type = Executor;
    using allocator_type = Allocator;
    using session_description = basic_session_description<Allocator>;
    using ice_candidate = basic_ice_candidate<Allocator>;
    using data_channel = basic_data_channel<Executor, Allocator>;

    // Fails as a value: error::too_many_peer_connections past the runtime's cap, the
    // platform's refusal (a bad server URL, a field section 7 refuses) as its code.
    static boost::system::result<basic_peer_connection> make(
        runtime& rt, Executor ex, configuration const& config,
        options const& opts = {}, Allocator alloc = {});

    basic_peer_connection(basic_peer_connection&&) noexcept;
    basic_peer_connection& operator=(basic_peer_connection&&) noexcept;
    ~basic_peer_connection();        // closes, aborts every pending operation

    executor_type get_executor() const noexcept;
    allocator_type get_allocator() const noexcept;

    // Operations, each with any completion token and terminal cancellation.
    template <class CompletionToken>
    auto async_set_local_description(CompletionToken&&);    // (error_code, session_description), implicit
    template <class CompletionToken>
    auto async_set_remote_description(session_description_view, CompletionToken&&);  // (error_code)
    template <class CompletionToken>
    auto async_add_ice_candidate(ice_candidate_view, CompletionToken&&);             // (error_code)

    // Events, each a pending operation the caller re-arms, as Asio's async_accept (section 5.2).
    template <class CompletionToken>
    auto async_wait_ice_candidate(CompletionToken&&);        // (error_code, ice_candidate); empty candidate: gathering ended
    template <class CompletionToken>
    auto async_wait_state_change(CompletionToken&&);         // (error_code, peer_connection_state)
    template <class CompletionToken>
    auto async_wait_ice_connection_state_change(CompletionToken&&);  // (error_code, ice_connection_state)
    template <class CompletionToken>
    auto async_accept_data_channel(CompletionToken&&);       // (error_code, data_channel)

    boost::system::result<data_channel> create_data_channel(std::string_view label,
                                                            data_channel_init const& = {});
    std::optional<session_description> local_description() const;
    bool has_remote_description() const;
    peer_connection_state state() const;
    ice_connection_state ice_state() const;
    void close();
};

template <class Executor, class Allocator>
class basic_data_channel {
public:
    using executor_type = Executor;
    using allocator_type = Allocator;

    basic_data_channel(basic_data_channel&&) noexcept;
    basic_data_channel& operator=(basic_data_channel&&) noexcept;
    ~basic_data_channel();           // closes, aborts every pending operation

    template <class CompletionToken>
    auto async_wait_open(CompletionToken&&);                 // (error_code); at once when open
    template <class CompletionToken>
    auto async_wait_close(CompletionToken&&);                // (error_code); at once when closed
    template <class DynamicBuffer, class CompletionToken>
    auto async_receive(DynamicBuffer&, CompletionToken&&);   // (error_code, std::size_t, bool is_text)
    template <class ConstBufferSequence, class CompletionToken>
    auto async_send(ConstBufferSequence const&, CompletionToken&&);  // (error_code), binary, once queued
    template <class CompletionToken>
    auto async_send_text(std::string_view, CompletionToken&&);       // (error_code)
    template <class CompletionToken>
    auto async_wait_buffered_amount_low(CompletionToken&&);  // (error_code); at once when at or below

    template <class ConstBufferSequence>
    boost::system::error_code send(ConstBufferSequence const&);      // binary, queued or failed
    boost::system::error_code send_text(std::string_view);

    void set_buffered_amount_low_threshold(std::size_t);
    std::size_t buffered_amount() const;
    data_channel_state ready_state() const;
    std::string_view label() const;  // in the object's allocator
    void close();
};

using session_description = basic_session_description<std::allocator<std::byte>>;
using ice_candidate = basic_ice_candidate<std::allocator<std::byte>>;
using peer_connection = basic_peer_connection<>;
using data_channel = basic_data_channel<boost::asio::any_io_executor, std::allocator<std::byte>>;

}
```

`rebind_alloc<A, T>` stands for `std::allocator_traits<A>::rebind_alloc<T>`.

Left out of the W3C API, each with its reason, in the table of differences (section 7):

- **`createOffer` and `createAnswer`.** libdatachannel marks them "for specific use cases
  only" (`rtc/peerconnection.hpp:104-106`), and its `setLocalDescription` takes a type, not
  a description (`:99`), so a description created that way cannot be applied as the W3C
  allows. The implicit `setLocalDescription`, the one trystero uses, does both.
- **`negotiationneeded`.** libdatachannel has no such event, only the query
  `negotiationNeeded()` (`rtc/peerconnection.hpp:89`). Natively, automatic negotiation is
  off (`Configuration::disableAutoNegotiation`, `rtc/configuration.hpp:77`, as trystero
  sets it at `native/peer_connection.hpp:96`), so on both targets a description exists only
  when `async_set_local_description` asks for one. The program negotiates when it adds a
  channel. A later version may add the event, synthesised natively.

## 5. Semantics

### 5.1 Handlers, threads and errors

- **Handlers** run on the object's executor, through its associated executor and allocator,
  as Asio's I/O objects run them, and never inside the call that started the operation:
  a completion known at once is posted.
- **Native threads.** libdatachannel's events arrive on its own threads. Each is posted to
  the executor, never run there, and nothing of the user's allocator is touched on those
  threads: the posts carry Asio's default allocator, and the queues of section 5.2 are
  filled on the executor (as trystero does today, `native/data_channel.hpp:43-45`). The
  allocation test counts only what runs on the executor.
- **Browser events** enter through webcpp::asio's bridge, which only posts to the
  io_context (the asio ruling of 2026-10-10: nothing calls `poll`, `run` or `dispatch` from
  a JavaScript event).
- **Errors** are values: `webcpp::webrtc::error`, an `error_category` named `webcpp.webrtc`,
  with fixed enumerator values that are never reused:

  | Value | Enumerator | When |
  | --- | --- | --- |
  | 1 | `invalid_state` | the W3C's `InvalidStateError`: a candidate before a remote description, a send on a channel that is not open |
  | 2 | `operation_error` | the W3C's `OperationError`; natively, any exception libdatachannel throws on an operation |
  | 3 | `invalid_access` | the W3C's `InvalidAccessError` |
  | 4 | `type_error` | the W3C's `TypeError`: both `max_packet_life_time` and `max_retransmits` |
  | 5 | `syntax_error` | the W3C's `SyntaxError`: an ICE server URL that cannot be parsed |
  | 6 | `closed` | the channel or the connection closed under a pending operation |
  | 7 | `message_too_large` | a message past `max_message_size`, sent or received |
  | 8 | `buffered_amount_exceeded` | a send that would pass `max_buffered_amount` |
  | 9 | `receive_overflow` | the browser's received bytes past `max_received_bytes` |
  | 10 | `too_many_peer_connections` | `make` past `runtime_options::max_peer_connections` |

  Asio's own codes keep their meaning: `boost::asio::error::operation_aborted` for an
  operation cancelled by `close()`, by destruction or by a cancellation slot;
  `boost::asio::error::already_started` for a second pending wait of one kind (section
  5.2); `boost::asio::error::operation_not_supported` for what a target cannot honour
  (section 7).
- **Cancellation.** Every operation supports terminal cancellation. In the browser, a
  cancelled `async_set_local_description` or `async_set_remote_description` cannot cancel
  the promise behind it: the description is still applied, and that effect may happen
  after the handler ran with `operation_aborted`. Natively the same holds for
  `async_set_local_description`, whose description libdatachannel produces on its thread.

### 5.2 Events

The events are pending operations, not callbacks: a caller waits for the next candidate,
state change or channel the way it waits for the next connection on an acceptor. This
keeps every handler on the executor and every lifetime in the caller's hands. What an
event that arrives with no operation pending becomes depends on its kind:

| Event | Kept while no wait is pending | Past the bound |
| --- | --- | --- |
| state change, ICE state change | the latest value, coalesced: a wait completes with the current state once it differs from the one the previous wait of that kind completed with (or `new_` for the first) | no bound needed |
| ICE candidate | a queue in the object's allocator, bounded by `max_queued_candidates` | the candidate is not queued; it is still in `local_description()`, which is what a program that does not trickle reads; the end of gathering is always queued, in a place the bound keeps for it |
| incoming data channel | a queue in the object's allocator, bounded by `max_queued_channels`, as a listen backlog | the channel is closed at once, as a connection past a listen backlog is refused |

- **End of candidates.** `async_wait_ice_candidate` completes successfully with an
  `ice_candidate` whose `candidate` is empty, the W3C's own end-of-candidates form, when
  gathering ends: from `onGatheringStateChange` reaching `Complete` natively
  (`rtc/peerconnection.hpp:122`), from an `icecandidate` event whose candidate is null or
  empty in the browser. It is not terminal: an ICE restart gathers again, and the next
  wait completes with the next candidate. `async_add_ice_candidate` accepts the empty
  candidate and adds none, as trystero's native backend does today
  (`native/peer_connection.hpp:205-223`).
- **One wait of a kind at a time.** A second pending wait of a kind on the same object
  completes at once with `already_started`; the first is unaffected.
- **A cancelled wait consumes no event:** the next wait of its kind gets the event the
  cancelled one would have had.
- **Waits that are already satisfied** complete at once (posted): `async_wait_open` on an
  open channel, `async_wait_close` on a closed one, `async_wait_buffered_amount_low` when
  `buffered_amount()` is at or below the threshold. In the browser `open` can fire before
  the caller waits, the case trystero handles at `detail/signaling.hpp:1035-1037`.
- **A channel's close.** When a data channel closes (the peer closed it, its connection
  closed, or its transport failed), its pending `async_wait_open`, `async_receive`,
  `async_send` and `async_wait_buffered_amount_low` complete with `error::closed`, and
  `async_wait_close` completes successfully; `ready_state()` reads `closed`. trystero ends a
  link on it (`detail/peer_link.hpp:80-89`, `detail/signaling.hpp:1026-1031`).
- **`close()`.** `basic_peer_connection::close()` closes the connection and its channels.
  As the W3C's `close()`, it fires no state change: every pending operation of the
  connection completes with `operation_aborted`, and `state()` reads `closed`. The
  connection's channels then close as above, with `error::closed`.
  `basic_data_channel::close()` completes the channel's pending operations with
  `operation_aborted`, except `async_wait_close`, which completes once the channel is
  closed.

### 5.3 Lifetime and outstanding work

- **The execution context outlives the object,** as for Asio's I/O objects. Natively this
  is what keeps a libdatachannel thread from posting to a destroyed `io_context`.
- **Destruction** closes the object and completes its pending operations with
  `operation_aborted`. Natively it calls `resetCallbacks()` on the `rtc::PeerConnection`
  and on every `rtc::DataChannel` before releasing them. That call synchronises with a
  callback in flight, because each callback runs under its own recursive mutex
  (`rtc/utils.hpp:53-100`), so once it returns nothing more is posted; what was posted
  before finds nothing, through a weak registration, as trystero's does today. In the
  browser, the object's registration with the bridge is removed and the JavaScript
  handlers are set to null (`browser/backend.hpp:515-527`).
- **Outstanding work.** A pending operation counts as outstanding work on its handler's
  executor, as Asio's operations do, so natively `run()` does not return while a candidate
  or a state change is awaited. A program that re-arms its waits keeps `run()` alive until
  it closes the connection; that is the way to end it.
- **The runtime.** libdatachannel keeps process-wide state: `Preload`, `Cleanup`,
  `SetThreadPoolSize` and `InitLogger` (`rtc/global.hpp:32-56`), and its threads must be
  stopped through `Cleanup()` before static destruction. webcpp keeps no global or static
  mutable state (AGENTS.md:638-639), so the program owns that state through a `runtime`:
  it makes one before any peer connection and destroys it after the last, and `make`
  takes it by reference, so no connection can exist without one. Natively, its
  constructor applies `thread_pool_size`, and its destructor waits for `rtc::Cleanup()`;
  at most one exists at a time in a process, because libdatachannel's state is the
  process's. Its other duty is the cap of peer connections, on both targets: natively it
  counts the connections made with it that are alive; in the browser it reads the page's
  census, which counts every `RTCPeerConnection` from its construction until the garbage
  collector frees it, because Chromium allows 500 per page and counts them that way
  (`browser/backend.hpp:914-926`). Past `max_peer_connections` (default 256, half of
  Chromium's limit), `make` fails with `too_many_peer_connections`. The census lives in
  JavaScript, as the page's, never in C++.

### 5.4 Messages and back-pressure

- **Receiving.** `async_receive` appends one whole message to the caller's dynamic buffer,
  copied once, and says whether it was text. Messages are kept from the channel's creation,
  so a channel accepted through `async_accept_data_channel` loses nothing it received before
  its first `async_receive`. The targets keep them differently:

  | | Native | Emscripten |
  | --- | --- | --- |
  | Where unread messages wait | libdatachannel's own queue, in its pull mode: no `onMessage` callback, and `receive()`, `availableAmount()` and `onAvailable` instead (`rtc/channel.hpp:49-53`) | a queue of the channel, in the object's allocator, filled from `onmessage` |
  | Its bound | 1,024 messages (`RECV_QUEUE_LIMIT`, libdatachannel `src/impl/internals.hpp:46` at v0.24.6), each at most `max_message_size` | `max_received_bytes`, in bytes |
  | Past it | libdatachannel's thread waits for room (`src/impl/queue.hpp:91-93`), SCTP stops reading, and the peer is slowed: real flow control, never a failure | the channel closes, and the pending or next `async_receive` completes with `receive_overflow` |

  The browser has no flow control on receipt, so a bound in bytes is the only protection.
  Natively, a channel that is never read holds one of libdatachannel's threads while its
  queue is full; increment 1 measures whether that stalls the connection's other channels,
  and the documentation says what it finds.
- **Message size.** A message larger than `max_message_size` (default 256 KiB) fails as a
  value with `message_too_large`. Natively, `max_message_size` is also libdatachannel's
  local `Configuration::maxMessageSize` (`rtc/configuration.hpp:90`), whose default it
  matches (`DEFAULT_LOCAL_MAX_MESSAGE_SIZE`, `src/impl/internals.hpp:41`), so a peer is
  told the limit in the SDP and libdatachannel refuses more. In the browser, which cannot
  announce it, a received message past it closes the channel, and the pending or next
  `async_receive` completes with `message_too_large`. A send past the smaller of
  `max_message_size` and the peer's announced limit fails without closing the channel.
- **Sending.** `send` and `send_text` queue the message and return; `async_send` and
  `async_send_text` complete once it is queued, which natively and in the browser is at
  once, posted. Both forms exist because both of trystero's backends send synchronously,
  fire and forget (`detail/peer_link.hpp:124-127`), as Asio's sockets offer `send` beside
  `async_send`. Each fails as a value: `invalid_state` when the channel is not open,
  `buffered_amount_exceeded` when the message would pass `max_buffered_amount` (default
  16 MiB). The caller paces itself with `async_wait_buffered_amount_low`.

### 5.5 Allocation

Every run-time object (the connection, its channels, their queues, the text of
`session_description`, `ice_candidate` and `label()`) is allocated through the object's
`Allocator`, on the executor. What libdatachannel and the browser hold is theirs: natively
the `rtc::` objects, their `std::string`s and the messages in their queue; in the browser
what `emscripten::val` holds, in the JavaScript heap. None of the types is shareable in the
sense of AGENTS.md:650-666: they are run-time objects.

## 6. Exceptions

By ruling 2.2.2:

- The native headers declare nothing under `WEBCPP_WEBRTC_NO_EXCEPTIONS`, with a comment
  that names libdatachannel. Each headers-alone call, test and program of the native
  backend declares `<exception-handling>on`, with the reason in a comment, since b2 lets no
  usage requirement override a build's `exception-handling=off` (AGENTS.md:745-761). The
  lint leaves those headers out of its analysis without exceptions, and names them.
- Inside the native backend, every call into libdatachannel that may throw is caught and
  becomes a value (`operation_error`, or the code section 5.1 names); nothing escapes to the
  caller.
- The emscripten backend raises no exception of its own, catches the browser's at the
  bridge, and is built and tested with exceptions on and off.

## 7. What a target cannot honour

What one target cannot do follows webcpp::beast's rule: `operation_not_supported` under
`WEBCPP_WEBRTC_UNSUPPORTED_ERROR` (the default) and ignored under
`WEBCPP_WEBRTC_UNSUPPORTED_IGNORE`. The documentation keeps the table of differences:

| What | Native | Emscripten |
| --- | --- | --- |
| `configuration::bind_address` | binds ICE to that address (libjuice only, `rtc/configuration.hpp:70`); needed for loopback candidates | not supported: `make` fails with `operation_not_supported`, or ignores it |
| `ice_candidate::sdp_m_line_index` on a local candidate | `rtc::Candidate` has only a mid (`rtc/candidate.hpp:24-40`): always `0`, which is right for a connection whose only m-line is the data channels', as trystero writes it (`native/peer_connection.hpp:342`) | the browser's |
| `ice_candidate::username_fragment` on a local candidate | empty: `rtc::Candidate` does not expose it | the browser's |
| a received message past `max_message_size` | refused by libdatachannel, announced in the SDP | closes the channel (section 5.4) |
| unread received messages | flow control, never a failure | bounded by `max_received_bytes` |
| `createOffer`, `createAnswer`, `negotiationneeded` | not offered (section 4) | not offered, for one interface |
| a native handle | none | none |

`negotiated` and `id` are in libdatachannel 0.24.6 (`rtc/peerconnection.hpp:31-36`) and in
the browser, so they are honoured on both.

## 8. Testing

Every lane runs on native and on emscripten, both suspension variants (`jspi` and
`asyncify`), at the CI level the owner's three levels give it.

- **Each target alone:** two peers in one process (natively) or one page (browser): offer,
  answer, candidates, the end of candidates, a channel, messages both ways (binary and
  text), back-pressure, close from each side, cancellation, destruction with operations
  pending. The cases section 5 names each have a test: two waits of one kind, a cancelled
  wait consuming nothing, `async_wait_open` on a channel already open, a pending receive
  completing with `closed`, `close()` firing no state change, the coalescing of states, the
  candidate and channel bounds, a message past `max_message_size`, the browser's
  `max_received_bytes`, `make` past the runtime's cap, a channel accepted that received
  before its first `async_receive`.
- **Native lifetime:** destruction while libdatachannel's threads are delivering events,
  repeated, under ThreadSanitizer; `run()` returning only once the connection is closed;
  the `runtime` stopping libdatachannel's threads before `main` returns.
- **Interop:** a native peer and a headless-Chrome peer, signalled through a local relay,
  exchange messages, in both suspension variants. Chrome runs with
  `--allow-loopback-in-peer-connection`, without which it offers no loopback candidate
  (`content/public/common/content_switches.cc`, `kAllowLoopbackInPeerConnection`), and with
  `--disable-features=WebRtcHideLocalIpsWithMdns`, without which it hides its host
  addresses behind mDNS `.local` names (the feature is on by default,
  `third_party/blink/common/features.cc`, `kWebRtcHideLocalIpsWithMdns`). The native peer
  sets `bind_address` to `127.0.0.1`: libdatachannel offers no loopback candidate otherwise
  (`native/peer_connection.hpp:76-79`).
- **Allocation:** a counting resource proves that the queues, the descriptions, the
  candidates and the labels use the object's allocator; natively it counts only what runs
  on the executor (section 5.1).
- **Exceptions:** the emscripten suites run whole with exceptions on and off. Natively the
  suites run with exceptions; without them, a headers-alone translation unit proves that
  including the native headers compiles, and a `webcpp.compile-diagnostic` test proves that
  naming `webcpp::webrtc::peer_connection` stops at that name.
- **No network beyond loopback:** ICE with host candidates only, no STUN, in CI.

## 9. libdatachannel

The native implementation wraps libdatachannel behind the interface above; nothing of
`rtc::` appears in a public header. Its documentation justifies it as AGENTS.md chapter 6
asks: no WebRTC exists in webcpp, Boost, the C++ Alliance or the standard library, and
libdatachannel is the smallest complete implementation (data channels without the media
stack), MPL-2.0, with its own OpenSSL or mbedTLS for DTLS. webcpp uses it unmodified, as a
dependency, and copies none of its files.

It is not pinned by webcpp today: like trystero, the library finds libdatachannel 0.24.6
where `-sLIBDATACHANNEL_ROOT=<dir>` or the superproject's `.local/libdatachannel` says,
which webcpp's CI installs (AGENTS.md:192-205, trystero's `build.jam:13-22`). It is pinned
when webcpp builds it from `third_party/` (chapter 13). Its MPL-2.0 licence awaits the
owner (section 11).

## 10. Increments

1. The interface, the `runtime` and the native implementation, its tests.
2. The browser implementation, its tests, the interop lane.
3. trystero on webcpp::webrtc (with webcpp::asio and webcpp::crypto), its `backend` gone;
   trystero keeps dropping text messages and messages larger than a room frame, and its
   per-minute window of connections (section 3).

The review's answers to the questions of the draft are taken: no zero-copy receive for now
(in the browser the bytes must leave the JavaScript heap anyway, and natively `rtc::binary`
is a `std::vector` in libdatachannel's allocator, so the one copy into the caller's buffer
is the copy that puts the bytes under the user's allocator); and separate event operations,
with the policies of section 5.2, rather than one wait returning a variant.

## 11. Open for the owner

- **libdatachannel's licence.** MPL-2.0 "is decided case by case, by the owner"
  (AGENTS.md:350-357), and no entry records that decision for libdatachannel. The owner's
  entry of 2026-10-10 names libdatachannel as the native implementation
  (WEBCPP_DESIGN.md:840), and webcpp uses it unmodified, so MPL's file-level copyleft
  reaches none of webcpp's files; the decision to record is that MPL-2.0 is accepted for
  this dependency.
- **The compile-time message without exceptions.** Ruling 2.2.2 asks that the native
  backend say at compile time that it is unavailable. AGENTS.md chapter 6 has the header
  declare nothing, so the message a program gets is the compiler's "no member named
  'peer_connection' in namespace 'webcpp::webrtc'". A clearer message (a declared stub
  whose use fails a `static_assert` naming libdatachannel and exceptions) would depart from
  chapter 6's "declares nothing"; the spec follows chapter 6 unless the owner wants the
  stub.
