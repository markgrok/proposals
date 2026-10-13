---
title: "webcpp::beast: Boost.Beast natively, the browser's WebSocket and fetch on Emscripten"
tags: [webcpp, beast, asio, emscripten, spec]
status: active
created: 2026-10-10
---

# webcpp::beast: specification (draft)

Sources cited below: `B/` is Boost 1.92's headers, `/opt/homebrew/include/boost`; `P/` is the
review's probes, `~/dev/webcpp/.superpowers/sdd/plan-reviews/asio-beast/`, among them Beast
1.92's own examples (`P/websocket_client_async.cpp`, `P/websocket_client_async_ssl.cpp`,
`P/http_client_async.cpp`); the review is `asio-beast-review.md` beside them. What the browser
does is the WHATWG WebSockets and Fetch Standards'.

## 1. Purpose

Code written against Boost.Beast, in its layers (resolve, connect, the TLS handshake, the
WebSocket handshake, then reads and writes), runs in a browser with the same code. Natively,
webcpp::beast is Boost.Beast itself. Compiled with Emscripten, the same names and
interfaces drive the browser's `WebSocket` and `fetch`: the layers the browser does not
expose record what the browser needs, and the last one opens the connection. What the
browser cannot honour fails as a value by default (decision 2), so a program that asks for it
(a client certificate, a custom header, a ping) learns so at the operation; section 4.6 lists,
for each of Beast's examples the twins use, what it needs under each setting. It is to
webcpp::asio what Boost.Beast is to Boost.Asio, and with webcpp::asio it is the prototype of a
proposal to Boost (section 10).

## 2. The owner's decisions (2026-10-10)

1. A library of its own, on webcpp::asio; WebSocket first, then the HTTP client.
2. What the browser cannot honour fails as a value by default,
   `WEBCPP_BEAST_UNSUPPORTED_ERROR`; a program may define
   `WEBCPP_BEAST_UNSUPPORTED_IGNORE` in its configuration to have it ignored (section 7).
3. The project's rules hold from the first commit (exceptions on and off, every variant,
   allocators, Boost first, outside interfaces wrapped, fixed-width types).
4. **The resolver** (the owner, 2026-10-10, option A of the review's C4): on Emscripten,
   webcpp::asio's resolver completes with entries that carry the host name and the service
   name and an unspecified endpoint; a plain socket's connect on such an endpoint fails with
   `operation_not_supported`, so webcpp::asio's "fails as a value" holds where a TCP
   connection would be needed; only webcpp::beast's browser streams accept such entries, so
   Beast's layered client code runs unchanged. A resolver of webcpp::beast's own, with
   webcpp::asio's failing like its sockets, was rejected: every Beast client would change a
   line.

## 3. Names

A program writes `namespace beast = webcpp::beast;` where it wrote
`namespace beast = boost::beast;`, and `webcpp::asio` where it wrote `boost::asio`, `ssl`
included (`namespace ssl = webcpp::asio::ssl;`): webcpp::asio owns the `ssl` names, which are
Boost's natively and in the browser a record of the program's TLS settings
(WEBCPP_ASIO_SPEC.md section 3), and webcpp::beast's browser streams read that record.
Everything of Beast that touches no network (buffers, `http` messages, parsers, serializers,
`websocket` close reasons and errors) is Beast's own on both targets.

| Name                                                    | Native                                                                       | Emscripten                                                                                                                                                                                 |
| ------------------------------------------------------- | ---------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `tcp_stream`                                            | Beast's                                                                      | a browser stream of webcpp's: records the resolver's entry, keeps Beast's timeout interface (`expires_after`, `expires_never`) on a Boost timer, and carries HTTP's bytes (section 5)      |
| `ssl_stream<tcp_stream>`                                | Beast's                                                                      | a browser stream of webcpp's: records the TLS settings and makes the scheme `wss` or `https`                                                                                               |
| `websocket::stream<NextLayer>`                          | a class derived from Beast's, adding the allocator constructor (section 4.4) | over the browser `tcp_stream` or `ssl_stream`: webcpp's browser WebSocket (section 4); over any other next layer, a byte stream (a relay socket, a WebRTC channel's): Beast's, as natively |
| `http::async_write`, `http::async_read` and the rest    | Beast's                                                                      | Beast's, unchanged, over the browser `tcp_stream`'s bytes (section 5)                                                                                                                      |
| `set_server_name(ssl_stream&, std::string_view host)`   | sets SNI through OpenSSL; returns `boost::system::result<void>`              | records nothing: the browser sends SNI itself; succeeds                                                                                                                                    |
| `teardown`, `async_teardown` for webcpp::asio's sockets | none needed: webcpp::asio's sockets are Boost's                              | declared in the sockets' namespace, so that Beast finds them by argument-dependent lookup (`B/beast/websocket/teardown.hpp:23-30`)                                                         |

Every other name is an alias of Boost.Beast's natively, so the two are the same types.
`set_server_name` replaces the one line of Beast's SSL examples that cannot compile in a
browser, `SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host)`
(`P/websocket_client_async_ssl.cpp:70`): there is no OpenSSL handle there. Boost 1.92 has no
such helper (no SNI function in `B/asio/ssl/` or `B/beast/`), so a program changes that line on
both targets.

## 4. The WebSocket client in the browser

### 4.1 The layers

| Layer, as Beast's code calls it | In the browser |
| --- | --- |
| `ip::tcp::resolver::async_resolve(host, service)` | webcpp::asio's resolver (decision 4) resolves nothing; completes with one entry, made with `basic_resolver_results::create(endpoint, host, service)` (`B/asio/ip/basic_resolver_results.hpp:159-166`, outside the documented interface: `GENERATING_DOCUMENTATION` guards it at lines 116-230, so a probe pins it), whose `host_name()` and `service_name()` are the arguments and whose endpoint has an unspecified address and the service's port: the number, or 80 for `ws` and `http`, 443 for `wss` and `https`; another service name fails with `boost::asio::error::service_not_found` |
| `get_lowest_layer(ws).expires_after(d)`, `expires_never()` | Beast's timeout on the stream's next operation, on a Boost timer: when it fires, the operation completes with `beast::error::timeout` and the connection is closed |
| `get_lowest_layer(ws).async_connect(results)` | records the first entry's host and service; completes at once |
| `get_lowest_layer(ws).async_connect(endpoint)` | records the endpoint's address as the host (an IPv6 address in brackets) and its port; an unspecified address fails with `error::no_host` (section 4.5): the browser connects by name or by address, and this one is neither |
| `ws.next_layer().async_handshake(ssl::stream_base::client)` | judges the recorded TLS settings (section 4.2) and records that the scheme is `wss`; the browser does TLS and verifies the certificate |
| `ws.async_handshake(host, target)` | opens `new WebSocket("<scheme>://<host>:<port><target>", subprotocols)` from the recorded host and port and the handshake's target, with `binaryType = "arraybuffer"` (the default, `"blob"`, is read asynchronously); completes on `open`, or with the error of section 4.5. The `host` argument is the `Host` field, which the browser sets itself: it is honoured when it equals the URL's host or `host:port`, and unsupported otherwise |
| `ws.async_read(buffer)` | completes with the next whole message, copied once into the dynamic buffer: binary from the `ArrayBuffer`, text with `TextEncoder.encodeInto` into the WebAssembly memory, the view taken again after a memory growth |
| `ws.async_read_some(buffer, n)`, `async_read_some(MutableBufferSequence)`, `is_message_done()` | the next message in pieces, of at most `n` bytes or the buffers' size; `is_message_done()` is true after its last piece |
| `ws.async_write(buffers)`, `text()`, `binary()`, `got_text()`, `got_binary()` | `send` of a string (decoded from UTF-8) or an `ArrayBuffer`; completes once handed to the browser (section 4.3) |
| `ws.async_write_some(fin, buffers)` | the pieces are buffered until `fin` and sent as one message: the peer receives the same message, and fragmentation is framing the browser does not expose; the buffer counts toward the send bound of section 4.3 |
| `ws.async_close(reason)` | `close(code, reason)`, completing on `close`; the codes the browser accepts, 1000 and 3000 to 4999 (WebSockets Standard, `close()`), are honoured, and any other (Beast's `going_away`, 1001, and the rest of `B/beast/websocket/rfc6455.hpp:78-111`) is unsupported. The reason fits: Beast's `reason_string` holds at most 123 characters (`rfc6455.hpp:155`) |
| `read_message_max` | enforced by the library: a longer message fails with `websocket::error::message_too_big` and closes the connection, without a code, since the browser refuses Beast's 1009 |

Every operation takes any completion token, through `boost::asio::async_initiate`, and its
handler runs on webcpp::asio's io_context, posted from the browser's events by webcpp::asio's
bridge (WEBCPP_ASIO_SPEC.md section 4.6).

### 4.2 What the browser cannot honour

Settings are classified by outcome: a setting is honoured where the browser does the same
thing, unsupported where it cannot, and ignored only where the browser does an equivalent
thing of its own, with the reason.

| Setting | Outcome |
| --- | --- |
| SNI, through `set_server_name` | honoured: the browser sends it |
| `ssl::verify_peer`, `ctx.set_default_verify_paths()` | honoured: the browser verifies against its own roots |
| `set_verify_callback(ssl::host_name_verification(h))` | honoured when `h` is the URL's host; unsupported otherwise. The browser stream tells it from any other callback by its type |
| a method that admits TLS 1.2 or later (`tls_client`, `tlsv12_client`, `sslv23_client`) | honoured: browsers negotiate TLS 1.2 or 1.3, never weaker |
| a method that excludes TLS 1.2 (`tlsv13_client`) or admits only older versions | unsupported: the browser may negotiate another version |
| `ssl::verify_none`, any other verify callback, a client certificate or private key (`use_certificate*`, `use_private_key*`), added roots (`add_certificate_authority`, `load_verify_file`, `add_verify_path`) | unsupported: the browser verifies by its own rules |
| `stream_base::timeout`: `handshake_timeout` | honoured |
| `stream_base::timeout`: `idle_timeout` with `keep_alive_pings` false | honoured: the connection closes after the interval without activity, as Beast's (`B/beast/websocket/stream_base.hpp:119-123`); activity is messages and close frames, since the browser answers the peer's pings itself and shows none (named in the table of differences) |
| `stream_base::timeout`: `idle_timeout` with `keep_alive_pings` true | unsupported: the browser sends no ping on request |
| `async_ping`, `async_pong` | unsupported |
| `control_callback` | honoured for close frames, whose code and reason the `close` event carries; pings and pongs are never reported |
| `set_option(stream_base::decorator)` | the library calls the decorator on an empty `websocket::request_type` at the handshake: `Sec-WebSocket-Protocol` becomes the constructor's subprotocols, honoured; any other field (`User-Agent`) is unsupported, since the browser sets its own |
| `permessage_deflate`, `auto_fragment`, `write_buffer_bytes` | ignored, on both settings: the browser negotiates compression and frames messages itself, and the messages the peer receives are the same |

"Unsupported" makes the operation that depends on the setting complete with
`boost::asio::error::operation_not_supported` under `WEBCPP_BEAST_UNSUPPORTED_ERROR` (the
default): the TLS handshake for a TLS setting, the WebSocket handshake for the decorator and
the timeout, the call itself for `async_ping`, `async_pong` and `async_close`; the connection
stays as it was. Under `WEBCPP_BEAST_UNSUPPORTED_IGNORE` the setting is ignored, and an
unsupported close code closes with 1000. Each case is named in the page's table of
differences.

`suggested(role_type::client)` is a 30 s handshake timeout, no idle timeout and no pings
(`B/beast/websocket/stream_base.hpp:149-153`), so it is honoured whole.

### 4.3 Back-pressure

- **Sending:** `send` never blocks and never refuses; the library bounds `bufferedAmount`
  (default 4 MiB, an option), and a write that would pass it fails with
  `error::send_limit` (section 4.5) and closes.
- **Receiving:** messages arrive whether a read is pending or not; they wait in a queue in the
  stream's allocator, bounded (an option), past which the stream closes, without a code, and
  the next read completes with `error::receive_limit`. A queued message does not keep
  `run` alive (WEBCPP_ASIO_SPEC.md section 4.8). `WebSocketStream`, where receiving would
  pause while C++ does not read, is a later increment (section 9): it exists in Chromium only
  and doubles the surface behind one interface.

### 4.4 Cancellation, lifetime and allocation

| Operation | `terminal` | `partial` | `total` |
| --- | --- | --- | --- |
| `async_handshake` | closes the browser socket (`close()`), completes with `operation_aborted` | as `terminal` | not supported: an opening cannot be undone |
| `async_read`, `async_read_some` | completes with `operation_aborted`; the message stays queued | as `terminal` | as `terminal`: nothing is consumed |
| `async_write`, `async_write_some` | no effect: the message is handed to the browser when the operation starts, and cannot be unsent | no effect | no effect |
| `async_close` | completes with `operation_aborted`; the browser's close goes on | as `terminal` | not supported |

The destructor completes every pending operation with `operation_aborted` (posted, so that
no pending work keeps `run` alive for good), closes the browser socket with `close()` without
a code, the nearest the browser has to a dropped connection, and removes its registrations
from webcpp::asio's bridge, so that a late event finds nothing.

Operation state uses the handler's associated allocator, as Beast's does. The stream's own
state (the receive queue, the registrations, the buffered fragments) uses the allocator given
to an extension of Beast's constructor, present on both targets:

```cpp
template <class Allocator, class... Args>
explicit stream(std::allocator_arg_t, const Allocator& allocator, Args&&... next_layer_args);
```

and `std::allocator<std::byte>` without it. Natively Beast's own state uses Beast's
allocation, which the documentation names; the counting test proves, in the browser on both
variants, that nothing else goes outside the given allocator.

### 4.5 Errors, by name

The browser's `error` event carries no cause, and a connection that fails closes with 1006
(WebSockets Standard), so refused connections, DNS failures, TLS failures and blocked ports
are one error. Failures with no counterpart in Beast or Asio are in webcpp::beast's own
category, `webcpp.beast`, with fixed values never reused:

| Browser outcome | `error_code` |
| --- | --- |
| the constructor throws `SyntaxError` (a URL or a subprotocol it rejects) | `webcpp::beast::error::invalid_url` (1) |
| the constructor throws `SecurityError` (mixed content, where a browser blocks it there) | `webcpp::beast::error::blocked` (2) |
| `error`, then `close` 1006, before `open` (refused, DNS, TLS, a blocked port, mixed content where a browser fails it later) | `webcpp::beast::error::connection_failed` (3) |
| the send bound passed (section 4.3) | `webcpp::beast::error::send_limit` (4) |
| the receive bound passed (section 4.3) | `webcpp::beast::error::receive_limit` (5) |
| `fetch` rejects with `TypeError` (network, CORS, mixed content), section 5 | `webcpp::beast::error::fetch_failed` (6) |
| `async_connect` on an endpoint with an unspecified address | `webcpp::beast::error::no_host` (7) |
| `close` with a code, after `open` | `websocket::error::closed`; `ws.reason()` holds the code and the reason, as Beast's |
| `close` 1006 after `open` | `boost::asio::error::connection_reset` |
| a timeout of section 4.1 or 4.2 | `beast::error::timeout`, as Beast's |
| a message past `read_message_max` | `websocket::error::message_too_big`, as Beast's |
| a setting the browser cannot honour | `boost::asio::error::operation_not_supported` (section 4.2) |

### 4.6 Beast's examples, setting by setting

| Example (Beast 1.92) | Under `ERROR` (the default) | Under `IGNORE` |
| --- | --- | --- |
| `websocket_client_async` | the decorator that sets `User-Agent` fails the handshake: the twin leaves it out | runs unchanged |
| `websocket_client_async_ssl` | the SNI line becomes `set_server_name` (`:70`); `host_name_verification` (`:80`), `tlsv12_client` and `verify_peer` (`:250-253`) and `suggested(client)` (`:147-149`) are honoured; the decorator (`:152-156`) and `load_root_certificates` (`:256`, added roots) fail the handshakes: the twin leaves the decorator out and calls `ctx.set_default_verify_paths()` in place of the added roots, which is honoured on both targets | the SNI line becomes `set_server_name`; the rest runs unchanged |
| `http_client_async` | `Host` (`:71`) is honoured when it is the URL's host; `User-Agent` (`:72`) fails the write: the twin leaves it out; `shutdown` (`:149`) is honoured (section 5) | runs unchanged |

## 5. The HTTP client in the browser (second increment)

The browser `tcp_stream` (and `ssl_stream`) is a byte stream for HTTP, so that Beast's own
algorithms run over it unchanged: `http::async_write` and `http::async_read`, `response_parser`,
`body_limit` and `async_read_header` included, and webcpp::beast overloads none of them.

- **Writing:** the bytes Beast's serializer writes are parsed back with Beast's
  `request_parser`; when a request is whole, the stream makes one `fetch` from it: the method,
  the URL from the recorded scheme, host and port and the request's target, the fields `fetch`
  allows, and the body, buffered whole (a chunked body is sent with its length: the server
  receives the same bytes). `redirect` is `"follow"` and `mode` is `"cors"`.
- **Reading:** the response is serialized with Beast's serializer into the bytes the stream's
  reads yield: the status line, with Beast's reason phrase for the status when the response
  has none (HTTP/2), the fields `fetch` exposes, and the body as it streams, framed by the
  stream with `Transfer-Encoding: chunked`. `Content-Length` and `Content-Encoding` are left
  out, since `fetch` hands over the body decoded.
- **Fields:** `Host` is honoured when it is the URL's host or `host:port` (the browser sets it
  from the URL), unsupported otherwise; `Content-Length` and `Transfer-Encoding` are the
  framing of the bytes, honoured; `Connection` and `Keep-Alive` are ignored, on both settings:
  connection reuse is the browser's, and the exchange is the same; `User-Agent` and every
  other field the browser forbids or sets itself is unsupported.
- **Redirects:** `fetch` follows them, and the program would see the final response, not the
  3xx it asked for: when `response.redirected` is true, the read completes with
  `operation_not_supported` under `ERROR`, and with the final response under `IGNORE`.
- **`CONNECT`**, and a request whose body the browser cannot send, are unsupported.
- **`stream.socket().shutdown(...)` and `close()`** end the stream: no further request is
  made, and they succeed. The browser stream's `socket()` is a member of webcpp::beast's
  that offers what Beast's examples call on it (`shutdown`, `close`, `is_open`, `cancel`,
  `get_executor`).
- **Errors:** a `fetch` that rejects completes the read with `error::fetch_failed`
  (section 4.5); CORS and mixed content are among its causes, which the browser does not tell
  apart.

The cost is one parse and one serialization per message, for Beast's code unchanged.

## 6. Natively

Nothing of the browser exists: webcpp::beast is the aliases of section 3, the derived
`websocket::stream` with its allocator constructor, and `set_server_name`. Beast's own tests
are Beast's; webcpp::beast tests that its names are Beast's types (or derive from them) and
that the guide's programs run.

## 7. Configuration

`WEBCPP_BEAST_UNSUPPORTED_ERROR` and `WEBCPP_BEAST_UNSUPPORTED_IGNORE` change inline code, so
two translation units that disagree would break the one definition rule. So:

- both are read in `<webcpp/beast/config.hpp>`, which every public header includes first,
  with `WEBCPP_BEAST_NO_EXCEPTIONS` (`AGENTS.md` chapter 6); `ERROR` is defined when neither
  is, and both defined is an `#error`;
- a b2 feature, `webcpp-beast-unsupported=error|ignore` (propagated), drives them from
  webcpp::beast's usage requirements
  (`<webcpp-beast-unsupported>ignore:<define>WEBCPP_BEAST_UNSUPPORTED_IGNORE`), so that a
  whole program builds with one setting;
- the browser classes live in an inline namespace named after the setting, as Asio tags its
  own (`B/asio/detail/config.hpp:1191-1195`), so that translation units that disagree name
  different types and fail at link, never silently;
- `test/config_test.cpp` is built under each setting, and with exceptions on and off.

## 8. Testing

- **Native lanes:** the names (static assertions: aliases of Beast's types, the stream derived
  from Beast's), the guide's programs against a local echo server.
- **The echo servers:** node has no WebSocket server built in, so the lanes use a native
  Beast echo server that the native lane builds (no npm dependency), plain and TLS; `wss`
  uses a local CA that node trusts through `NODE_EXTRA_CA_CERTS` and headless Chrome through
  the lane's certificate options.
- **Emscripten, both suspension variants, under node:** node 26 has `WebSocket`, `fetch` and
  `MessageChannel`, not `WebSocketStream`, and enforces neither CORS nor mixed content
  (measured, review). These lanes test the layers of section 4.1, the URL and `Host` rules,
  the settings of section 4.2 under both configurations, cancellation and the destructor
  (section 4.4), back-pressure through the queue (a writer faster than the server, a reader
  that does not read), a server that closes, refuses, or sends a message past
  `read_message_max`, every error of section 4.5 that node can produce, allocation counts, and
  the HTTP client against the native server.
- **The browser lane:** the same programs in headless Chrome, and what only a browser shows:
  CORS and mixed-content failures, and, with its increment, `WebSocketStream`.
- **Twins:** each example of the guide prints the same natively and in the browser, Beast's
  three examples of section 4.6 among them, as that section adapts them.

## 9. Increments

1. The WebSocket client, both targets, the queue path.
2. The HTTP client in the browser.
3. `WebSocketStream` where the browser has it.
4. trystero's relays on webcpp::beast.

## 10. Toward Boost

webcpp::asio and webcpp::beast are the proven prototype of a proposal. Emscripten 6.0.11
already makes Boost.Asio's `io_context::run` suspend into the browser
(WEBCPP_ASIO_SPEC.md section 4.1), so the proposal to Boost.Asio is what webcpp::asio adds
(the threads configuration on Emscripten, the turn limit, the wake from browser events), and
the proposal to Boost.Beast is browser streams, so that Beast's layered code drives
`WebSocket` and `fetch`. The proposal is made when both have run in real programs, trystero
first.
