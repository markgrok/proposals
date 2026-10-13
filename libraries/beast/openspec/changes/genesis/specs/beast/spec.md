## Purpose

webcpp::beast lets code written against Boost.Beast, in its layers (resolve, connect, the TLS handshake, the WebSocket handshake, then reads and writes), run in a browser with the same code. Natively it is Boost.Beast itself. Compiled with Emscripten, the same names and interfaces drive the browser's `WebSocket` and `fetch`: the layers the browser does not expose record what the browser needs, and the last one opens the connection. What the browser cannot honour fails as a value by default. It is to webcpp::asio what Boost.Beast is to Boost.Asio. These requirements are the structured form of `reference/spec.md` (draft of 2026-10-10); the library is not built, so nothing here has been run.

## ADDED Requirements

### Requirement: Library: header-only library in its namespace
The library SHALL be header-only C++20, in namespace `webcpp::beast`, with public headers under `<webcpp/beast/...>`, one per responsibility, macros prefixed `WEBCPP_BEAST_`, on native and Emscripten targets. It MUST NOT declare a WASI target, because webcpp::asio, on which it builds, has none (Boost.Asio 1.92 does not compile for wasm32-wasip2 or wasm32-wasip3 with wasi-sdk 34).

#### Scenario: A program names the library like Boost.Beast
- **WHEN** a program writes `namespace beast = webcpp::beast;` where it wrote `namespace beast = boost::beast;`
- **THEN** it compiles on native and on Emscripten with the names of Boost.Beast's layered client code

### Requirement: Library: build target and dependencies
`build.jam` SHALL declare `project /webcpp/beast` and one alias `beast` whose usage requirements include the library's headers, webcpp::asio, Boost's headers and `/webcpp/config//config`. The alias MUST carry the `webcpp-beast-unsupported` feature's define (see the Config requirements) in its usage requirements.

#### Scenario: A program uses the library without building it
- **WHEN** a b2 program names `/webcpp/beast//beast` and includes a public header
- **THEN** it compiles and links without any library of webcpp::beast's own being built

### Requirement: Library: repository files of a webcpp library
The repository SHALL hold `build.jam`, `meta/libraries.json` (`"port-of": null`), `README.md`, `AGENTS.md` (only what is specific to beast), `LICENSE_1_0.txt` (BSL-1.0, "Copyright (c) 2026 WebCpp.org" on every source file), `doc/changelog.adoc`, and a CI workflow calling the superproject's.

#### Scenario: The superproject's lint accepts the layout
- **WHEN** the superproject's lint reads `libs/beast`
- **THEN** it finds each of these files and reports no rule failed

### Requirement: Library: the project's rules hold from the first commit
The library SHALL be built and tested with exceptions on and off (`WEBCPP_BEAST_NO_EXCEPTIONS`), on both Emscripten suspension variants, with allocators the program gives, Boost before custom code, outside interfaces wrapped and fixed-width types. Errors MUST be values; an exception is raised only through `boost::throw_exception` and only with exceptions enabled.

#### Scenario: The library works without exceptions
- **WHEN** a program using the WebSocket client is built with exceptions off and a connection fails
- **THEN** the handler receives an `error_code`, and nothing is thrown

### Requirement: Names: Beast's network-free names are Beast's own
Everything of Boost.Beast that touches no network (buffers, `http` messages, parsers, serializers, `websocket` close reasons and errors) SHALL be Beast's own on both targets, and every other name not listed in the Names requirements MUST be an alias of Boost.Beast's natively, so that the two are the same types.

#### Scenario: An alias is the same type
- **WHEN** a native test compares `webcpp::beast::flat_buffer` with `boost::beast::flat_buffer` through `std::is_same_v`
- **THEN** the assertion holds

### Requirement: Names: ssl names are webcpp::asio's
A program SHALL write `webcpp::asio` where it wrote `boost::asio`, `ssl` included (`namespace ssl = webcpp::asio::ssl;`). webcpp::asio owns the `ssl` names, which are Boost's natively and in the browser a record of the program's TLS settings, and webcpp::beast's browser streams MUST read that record.

#### Scenario: The browser stream reads the recorded settings
- **WHEN** on Emscripten a program sets `ctx.set_verify_mode(ssl::verify_peer)` on a `webcpp::asio::ssl::context` and passes it to an `ssl_stream<tcp_stream>`
- **THEN** the stream's TLS handshake judges that recorded setting

### Requirement: Names: tcp_stream
`tcp_stream` SHALL be Beast's own natively. On Emscripten it MUST be a browser stream of webcpp's that records the resolver's entry, keeps Beast's timeout interface (`expires_after`, `expires_never`) on a Boost timer, and carries HTTP's bytes (see the HTTP requirements).

#### Scenario: Native tcp_stream is Beast's
- **WHEN** a native test compares `webcpp::beast::tcp_stream` with `boost::beast::tcp_stream`
- **THEN** they are the same type

### Requirement: Names: ssl_stream
`ssl_stream<tcp_stream>` SHALL be Beast's own natively. On Emscripten it MUST be a browser stream of webcpp's that records the TLS settings and makes the scheme `wss` or `https`.

#### Scenario: The TLS stream makes the scheme secure
- **WHEN** on Emscripten `ws.next_layer().async_handshake(ssl::stream_base::client)` succeeds on a `websocket::stream<ssl_stream<tcp_stream>>`
- **THEN** the WebSocket handshake opens a URL whose scheme is `wss`

### Requirement: Names: websocket::stream natively
`websocket::stream<NextLayer>` SHALL natively be a class derived from Beast's, adding the allocator constructor `template <class Allocator, class... Args> explicit stream(std::allocator_arg_t, const Allocator& allocator, Args&&... next_layer_args)`, which is present on both targets.

#### Scenario: The native stream derives from Beast's
- **WHEN** a native static assertion tests `std::is_base_of_v<boost::beast::websocket::stream<boost::beast::tcp_stream>, webcpp::beast::websocket::stream<webcpp::beast::tcp_stream>>`
- **THEN** it holds

### Requirement: Names: websocket::stream on Emscripten is selected by its next layer
On Emscripten, `websocket::stream<NextLayer>` over the browser `tcp_stream` or `ssl_stream` SHALL be webcpp's browser WebSocket. Over any other next layer (a relay socket, a WebRTC channel's byte stream) it MUST be Beast's, as natively.

#### Scenario: A byte stream gets Beast's WebSocket
- **WHEN** on Emscripten a program instantiates `websocket::stream` over a user byte stream
- **THEN** the instantiation is Beast's implementation, which frames the bytes itself

### Requirement: Names: HTTP algorithms
`http::async_write`, `http::async_read` and the rest of Beast's HTTP algorithms SHALL be Beast's, unchanged, on both targets; webcpp::beast MUST overload none of them. In the browser they run over the browser `tcp_stream`'s bytes.

#### Scenario: No overload of async_read
- **WHEN** the headers of webcpp::beast are searched for a declaration of an `http::async_read` overload
- **THEN** there is none

### Requirement: Names: set_server_name
The library SHALL provide `set_server_name(ssl_stream&, std::string_view host)` returning `boost::system::result<void>`. Natively it MUST set SNI through OpenSSL; on Emscripten it MUST record nothing, because the browser sends SNI itself, and MUST succeed. It replaces `SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host)`, since Boost 1.92 has no SNI helper and there is no OpenSSL handle in a browser.

#### Scenario: SNI set in the browser
- **WHEN** on Emscripten a program calls `set_server_name(ws.next_layer(), "echo.example.org")`
- **THEN** the result is a value holding no error, and nothing is recorded

### Requirement: Names: teardown for webcpp::asio's sockets
`teardown` and `async_teardown` for webcpp::asio's sockets SHALL be declared in the sockets' namespace, so that Beast finds them by argument-dependent lookup. Natively none is needed, because webcpp::asio's sockets are Boost's.

#### Scenario: Beast finds the teardown
- **WHEN** a `websocket::stream` over a webcpp::asio socket is closed on Emscripten
- **THEN** Beast's `async_teardown` call resolves to webcpp::beast's declaration by argument-dependent lookup

### Requirement: Resolver: the browser streams accept webcpp::asio's entry
On Emscripten the browser streams SHALL accept the single entry webcpp::asio's resolver completes with, made with `basic_resolver_results::create(endpoint, host, service)`: `host_name()` and `service_name()` are the arguments, and the endpoint has an unspecified address and the service's port. The port MUST be the number given, or 80 for `ws` and `http`, 443 for `wss` and `https`; another service name fails with `boost::asio::error::service_not_found`. A plain socket's connect on such an endpoint fails with `operation_not_supported` (decision of the owner, 2026-10-10, option A).

#### Scenario: The resolver's port for a scheme
- **WHEN** on Emscripten `resolver.async_resolve("echo.example.org", "wss")` completes
- **THEN** the single entry's `service_name()` is `"wss"` and its endpoint port is 443

#### Scenario: Another service name
- **WHEN** `resolver.async_resolve("echo.example.org", "gopher")` is called
- **THEN** it completes with `boost::asio::error::service_not_found`

### Requirement: WebSocket: connecting records, it does not connect
`get_lowest_layer(ws).async_connect(results)` SHALL record the first entry's host and service and complete at once. `async_connect(endpoint)` MUST record the endpoint's address as the host (an IPv6 address in brackets) and its port, and an endpoint with an unspecified address MUST fail with `webcpp::beast::error::no_host`, because the browser connects by name or by address and this one is neither.

#### Scenario: An IPv6 endpoint
- **WHEN** `async_connect` is called with the endpoint `[::1]:8080`
- **THEN** the recorded host is `[::1]` and the recorded port is 8080

#### Scenario: An unspecified address
- **WHEN** `async_connect` is called with an endpoint whose address is unspecified
- **THEN** it completes with `webcpp::beast::error::no_host`

### Requirement: WebSocket: the TLS handshake judges the record
`ws.next_layer().async_handshake(ssl::stream_base::client)` SHALL judge the recorded TLS settings (see the Settings requirements) and record that the scheme is `wss`; the browser does TLS and verifies the certificate.

#### Scenario: A client certificate is refused
- **WHEN** under `WEBCPP_BEAST_UNSUPPORTED_ERROR` a program calls `ctx.use_certificate_file(...)` and then the TLS handshake
- **THEN** the handshake completes with `boost::asio::error::operation_not_supported` and the connection stays as it was

### Requirement: WebSocket: the handshake opens the browser WebSocket
`ws.async_handshake(host, target)` SHALL open `new WebSocket("<scheme>://<host>:<port><target>", subprotocols)` from the recorded host and port and the handshake's target, with `binaryType = "arraybuffer"`, and complete on `open` or with the error of the Errors requirements. The `host` argument is the `Host` field, which the browser sets itself: it MUST be honoured when it equals the URL's host or `host:port`, and unsupported otherwise.

#### Scenario: Host equals the URL's host
- **WHEN** the recorded host is `echo.example.org`, port 8080, and `async_handshake("echo.example.org:8080", "/")` is called
- **THEN** the browser WebSocket is opened for `ws://echo.example.org:8080/` and the handshake completes on `open`

#### Scenario: A different Host
- **WHEN** under `ERROR` `async_handshake("other.example.org", "/")` is called for the recorded host `echo.example.org`
- **THEN** it completes with `operation_not_supported`

### Requirement: WebSocket: reading a whole message
`ws.async_read(buffer)` SHALL complete with the next whole message, copied once into the dynamic buffer: binary from the `ArrayBuffer`, text with `TextEncoder.encodeInto` into the WebAssembly memory, the view taken again after a memory growth. `got_text()` and `got_binary()` report the message's kind.

#### Scenario: A text message
- **WHEN** the server sends the text message `hello` and the program reads it into a `flat_buffer`
- **THEN** the buffer holds the five bytes `hello` and `got_text()` is true

### Requirement: WebSocket: reading in pieces
`ws.async_read_some(buffer, n)` and `async_read_some(MutableBufferSequence)` SHALL return the next message in pieces of at most `n` bytes, or the buffers' size, and `is_message_done()` MUST be true after the last piece.

#### Scenario: A message in two pieces
- **WHEN** a 10-byte message is read with `async_read_some(buffer, 6)` twice
- **THEN** the pieces are 6 and 4 bytes and `is_message_done()` is false after the first and true after the second

### Requirement: WebSocket: writing a message
`ws.async_write(buffers)` SHALL send a string (decoded from UTF-8) when `text()` is set and an `ArrayBuffer` when `binary()` is, and complete once the message is handed to the browser (it counts toward the send bound).

#### Scenario: A binary message
- **WHEN** a program calls `ws.binary(true)` and writes the three bytes `01 02 03`
- **THEN** the peer receives one binary message of those bytes

### Requirement: WebSocket: partial writes become one message
`ws.async_write_some(fin, buffers)` SHALL buffer the pieces until `fin` is true and then send them as one message: the peer receives the same message, and fragmentation is framing the browser does not expose. The buffered bytes MUST count toward the send bound.

#### Scenario: Two pieces, one message
- **WHEN** a program writes `ab` with `fin` false and then `cd` with `fin` true
- **THEN** the peer receives one message `abcd`

### Requirement: WebSocket: closing
`ws.async_close(reason)` SHALL call the browser's `close(code, reason)` and complete on `close`. The codes the browser accepts, 1000 and 3000 to 4999, are honoured; any other (Beast's `going_away`, 1001, and the rest of the codes of `boost/beast/websocket/rfc6455.hpp`) is unsupported. The reason fits: Beast's `reason_string` holds at most 123 characters.

#### Scenario: A code the browser refuses
- **WHEN** under `ERROR` a program calls `async_close(websocket::close_code::going_away)`
- **THEN** the call completes with `operation_not_supported` and the connection stays open

#### Scenario: A close with an unsupported code under IGNORE
- **WHEN** under `IGNORE` the same call is made
- **THEN** the connection closes with code 1000

### Requirement: WebSocket: the longest message
`read_message_max` SHALL be enforced by the library: a longer message fails with `websocket::error::message_too_big` and closes the connection without a code, since the browser refuses Beast's 1009.

#### Scenario: A message past the limit
- **WHEN** `read_message_max` is 1024 and the server sends 2048 bytes in one message
- **THEN** the read completes with `websocket::error::message_too_big` and the connection is closed

### Requirement: WebSocket: timeouts on a Boost timer
`get_lowest_layer(ws).expires_after(d)` and `expires_never()` SHALL keep Beast's timeout interface on the stream's next operation, on a Boost timer: when it fires, the operation completes with `beast::error::timeout` and the connection is closed.

#### Scenario: A read that outlasts expires_after
- **WHEN** a program calls `expires_after(100ms)` and starts a read the server never answers
- **THEN** the read completes with `beast::error::timeout` and the browser socket is closed

### Requirement: WebSocket: completion tokens and the executor
Every operation SHALL take any completion token, through `boost::asio::async_initiate`, and its handler MUST run on webcpp::asio's io_context, posted from the browser's events by webcpp::asio's bridge.

#### Scenario: A handler runs on the io_context
- **WHEN** the browser's `open` event fires for a pending handshake
- **THEN** the handshake's handler is posted to the stream's executor and runs inside `io_context::run`, never inside the event

### Requirement: Settings: every setting is classified
Each setting of Beast's and Asio's that a program can set SHALL be classified by outcome: honoured where the browser does the same thing, unsupported where it cannot, and ignored only where the browser does an equivalent thing of its own, with the reason. Each case MUST be named in the documentation's table of differences.

#### Scenario: The table names every case
- **WHEN** the documentation page is built
- **THEN** its table of differences lists each unsupported and each ignored setting of this specification

### Requirement: Settings: TLS settings the browser honours
SNI through `set_server_name`, `ssl::verify_peer`, `ctx.set_default_verify_paths()`, `set_verify_callback(ssl::host_name_verification(h))` when `h` is the URL's host, and a method that admits TLS 1.2 or later (`tls_client`, `tlsv12_client`, `sslv23_client`) SHALL be honoured. The browser stream MUST tell `host_name_verification` from any other callback by its type.

#### Scenario: Host name verification for the URL's host
- **WHEN** a program sets `ssl::host_name_verification("echo.example.org")` and connects to that host
- **THEN** the TLS handshake succeeds

### Requirement: Settings: TLS settings the browser cannot honour
A method that excludes TLS 1.2 (`tlsv13_client`) or admits only older versions, `ssl::verify_none`, any other verify callback, a client certificate or private key (`use_certificate*`, `use_private_key*`), added roots (`add_certificate_authority`, `load_verify_file`, `add_verify_path`), and `host_name_verification` for a host other than the URL's SHALL be unsupported, because the browser verifies by its own rules.

#### Scenario: TLS 1.3 only
- **WHEN** under `ERROR` a program uses `ssl::context ctx{ssl::context::tlsv13_client}` and runs the TLS handshake
- **THEN** it completes with `operation_not_supported`

### Requirement: Settings: stream timeouts
`stream_base::timeout` SHALL be honoured for `handshake_timeout`, and for `idle_timeout` with `keep_alive_pings` false: the connection closes after the interval without activity, as Beast's, where activity is messages and close frames (the browser answers the peer's pings itself and shows none). `idle_timeout` with `keep_alive_pings` true MUST be unsupported, because the browser sends no ping on request. `suggested(role_type::client)`, a 30 s handshake timeout with no idle timeout and no pings, MUST be honoured whole.

#### Scenario: Idle timeout with pings
- **WHEN** under `ERROR` a program sets a timeout with `keep_alive_pings` true and runs the WebSocket handshake
- **THEN** the handshake completes with `operation_not_supported`

#### Scenario: suggested client timeout
- **WHEN** a program sets `stream_base::timeout::suggested(role_type::client)`
- **THEN** the handshake completes normally and a handshake slower than 30 s fails with `beast::error::timeout`

### Requirement: Settings: ping, pong and control callbacks
`async_ping` and `async_pong` SHALL be unsupported. `control_callback` MUST be honoured for close frames, whose code and reason the `close` event carries; pings and pongs are never reported.

#### Scenario: A ping
- **WHEN** under `ERROR` a program calls `ws.async_ping({})`
- **THEN** it completes with `operation_not_supported` and the connection stays as it was

### Requirement: Settings: the decorator
`set_option(stream_base::decorator)` SHALL be called by the library on an empty `websocket::request_type` at the handshake: `Sec-WebSocket-Protocol` becomes the constructor's subprotocols and is honoured; any other field (`User-Agent`) is unsupported, since the browser sets its own.

#### Scenario: A subprotocol in the decorator
- **WHEN** the decorator sets `Sec-WebSocket-Protocol: chat`
- **THEN** the browser WebSocket is opened with the subprotocol `chat`

#### Scenario: A User-Agent in the decorator
- **WHEN** under `ERROR` the decorator sets `User-Agent`
- **THEN** the WebSocket handshake completes with `operation_not_supported`

### Requirement: Settings: ignored options
`permessage_deflate`, `auto_fragment` and `write_buffer_bytes` SHALL be ignored, on both settings, because the browser negotiates compression and frames messages itself, and the messages the peer receives are the same.

#### Scenario: Compression requested
- **WHEN** a program enables `permessage_deflate` and writes a message
- **THEN** no error is reported and the peer receives the same message

### Requirement: Settings: ERROR and IGNORE
Under `WEBCPP_BEAST_UNSUPPORTED_ERROR` (the default) an unsupported setting SHALL make the operation that depends on it complete with `boost::asio::error::operation_not_supported` (the TLS handshake for a TLS setting, the WebSocket handshake for the decorator and the timeout, the call itself for `async_ping`, `async_pong` and `async_close`), and the connection MUST stay as it was. Under `WEBCPP_BEAST_UNSUPPORTED_IGNORE` the setting MUST be ignored, and an unsupported close code closes with 1000.

#### Scenario: Same program, two settings
- **WHEN** a program that sets `User-Agent` in its decorator is built once with each setting
- **THEN** the handshake fails with `operation_not_supported` under `ERROR` and succeeds under `IGNORE`

### Requirement: Back-pressure: sending is bounded
`send` SHALL never block and never refuse; the library MUST bound `bufferedAmount` (default 4 MiB, an option), and a write that would pass the bound fails with `webcpp::beast::error::send_limit` and closes the connection.

#### Scenario: A writer faster than the server
- **WHEN** a program writes messages in a loop to a server that does not read, until `bufferedAmount` would pass 4 MiB
- **THEN** that write completes with `error::send_limit` and the stream is closed

### Requirement: Back-pressure: receiving is queued and bounded
Messages SHALL arrive whether a read is pending or not and wait in a queue in the stream's allocator, bounded by an option; past the bound the stream closes without a code and the next read completes with `webcpp::beast::error::receive_limit`. A queued message MUST NOT keep `io_context::run` alive.

#### Scenario: A reader that does not read
- **WHEN** the server sends messages past the queue bound while the program reads none
- **THEN** the stream closes without a code and the next `async_read` completes with `error::receive_limit`

#### Scenario: A queued message and run
- **WHEN** a message is queued and no operation is pending
- **THEN** `io_context::run` returns

### Requirement: Lifetime: cancellation by type
Cancellation SHALL act per operation and per cancellation type. `async_handshake` under `terminal` or `partial` closes the browser socket (`close()`) and completes with `operation_aborted`, and under `total` is not supported, since an opening cannot be undone. `async_read` and `async_read_some` complete with `operation_aborted` under every type and the message stays queued. `async_write` and `async_write_some` are not affected, since the message is handed to the browser when the operation starts. `async_close` under `terminal` or `partial` completes with `operation_aborted` while the browser's close goes on, and under `total` is not supported.

#### Scenario: A cancelled read keeps its message
- **WHEN** a pending `async_read` is cancelled with `terminal` and a message then arrives
- **THEN** the read completed with `operation_aborted`, and the next `async_read` returns that message

### Requirement: Lifetime: the destructor
The destructor SHALL complete every pending operation with `operation_aborted` (posted, so that no pending work keeps `run` alive for good), close the browser socket with `close()` without a code, and remove its registrations from webcpp::asio's bridge, so that a late event finds nothing.

#### Scenario: A late event after destruction
- **WHEN** a stream is destroyed with a read pending and the browser then delivers a message for it
- **THEN** the read handler ran with `operation_aborted` and the late event finds no registration and does nothing

### Requirement: Allocation: operations and stream state
Operation state SHALL use the handler's associated allocator, as Beast's does. The stream's own state (the receive queue, the registrations, the buffered fragments) MUST use the allocator given to the allocator constructor, and `std::allocator<std::byte>` without it. Natively Beast's own state uses Beast's allocation, which the documentation names.

#### Scenario: The counting test
- **WHEN** a counting resource is passed to the stream's allocator constructor and a session of handshake, writes, reads and close runs in the browser on both variants
- **THEN** no allocation of the stream's state goes outside the given allocator

### Requirement: Errors: the category webcpp.beast
Failures with no counterpart in Beast or Asio SHALL be in webcpp::beast's own category, named `webcpp.beast`, with fixed values never reused: `invalid_url` (1), `blocked` (2), `connection_failed` (3), `send_limit` (4), `receive_limit` (5), `fetch_failed` (6), `no_host` (7), as members of `webcpp::beast::error`.

#### Scenario: Values are fixed
- **WHEN** a test reads the integer value of each `webcpp::beast::error` enumerator
- **THEN** they are 1 to 7 in the order above, and the category's `name()` is `"webcpp.beast"`

### Requirement: Errors: browser outcomes by name
The browser's outcomes SHALL map to error codes: a constructor throwing `SyntaxError` (a URL or subprotocol it rejects) to `invalid_url`; one throwing `SecurityError` to `blocked`; `error` then `close` 1006 before `open` (refused, DNS, TLS, a blocked port) to `connection_failed`; `close` with a code after `open` to `websocket::error::closed`, with `ws.reason()` holding the code and the reason, as Beast's; `close` 1006 after `open` to `boost::asio::error::connection_reset`; a timeout to `beast::error::timeout`; a message past `read_message_max` to `websocket::error::message_too_big`; a setting the browser cannot honour to `boost::asio::error::operation_not_supported`.

#### Scenario: A refused connection
- **WHEN** the server refuses the connection and the browser reports `error` then `close` 1006 before `open`
- **THEN** the handshake completes with `webcpp::beast::error::connection_failed`

#### Scenario: The peer closes with a code
- **WHEN** the server closes with code 4000 and reason `bye` after `open`
- **THEN** a pending read completes with `websocket::error::closed`, and `ws.reason().code` is 4000 and its reason is `bye`

#### Scenario: An invalid URL
- **WHEN** the recorded host is not a valid URL host and the handshake is made
- **THEN** it completes with `webcpp::beast::error::invalid_url`

### Requirement: HTTP: a byte stream for Beast's algorithms
The browser `tcp_stream` and `ssl_stream` SHALL be a byte stream for HTTP, so that Beast's own algorithms run over it unchanged: `http::async_write`, `http::async_read`, `response_parser`, `body_limit` and `async_read_header` included. This is the second increment.

#### Scenario: Beast's client over fetch
- **WHEN** the twin of Beast's `http_client_async` runs in the browser against a local server
- **THEN** it prints the same response as natively

### Requirement: HTTP: writing makes one fetch
The bytes Beast's serializer writes SHALL be parsed back with Beast's `request_parser`; when a request is whole, the stream MUST make one `fetch` from it: the method, the URL from the recorded scheme, host and port and the request's target, the fields `fetch` allows, and the body, buffered whole (a chunked body is sent with its length: the server receives the same bytes). `redirect` is `"follow"` and `mode` is `"cors"`.

#### Scenario: A GET
- **WHEN** a program writes `GET /index.html HTTP/1.1` with `Host` equal to the recorded host
- **THEN** one `fetch` is made for `<scheme>://<host>:<port>/index.html` with method GET, `redirect: "follow"` and `mode: "cors"`

### Requirement: HTTP: reading serializes the response
The response SHALL be serialized with Beast's serializer into the bytes the stream's reads yield: the status line, with Beast's reason phrase for the status when the response has none (HTTP/2), the fields `fetch` exposes, and the body as it streams, framed by the stream with `Transfer-Encoding: chunked`. `Content-Length` and `Content-Encoding` MUST be left out, since `fetch` hands over the body decoded.

#### Scenario: A response without a reason phrase
- **WHEN** `fetch` resolves with status 200 and an empty status text
- **THEN** the bytes read begin `HTTP/1.1 200 OK` and carry `Transfer-Encoding: chunked`

### Requirement: HTTP: request fields
`Host` SHALL be honoured when it is the URL's host or `host:port` (the browser sets it from the URL), unsupported otherwise. `Content-Length` and `Transfer-Encoding` are the framing of the bytes and MUST be honoured. `Connection` and `Keep-Alive` MUST be ignored, on both settings, since connection reuse is the browser's and the exchange is the same. `User-Agent` and every other field the browser forbids or sets itself SHALL be unsupported.

#### Scenario: User-Agent
- **WHEN** under `ERROR` a request carries `User-Agent: beast`
- **THEN** the write completes with `operation_not_supported`

#### Scenario: Connection is ignored
- **WHEN** a request carries `Connection: close`
- **THEN** no error is reported on either setting and the fetch is made

### Requirement: HTTP: redirects
`fetch` follows redirects and the program would see the final response, not the 3xx it asked for. When `response.redirected` is true, the read SHALL complete with `operation_not_supported` under `ERROR`, and with the final response under `IGNORE`.

#### Scenario: A redirected response
- **WHEN** the server answers a request with a 302 to another path and the final response is 200
- **THEN** under `ERROR` the read completes with `operation_not_supported`, and under `IGNORE` it yields the 200 response

### Requirement: HTTP: methods and bodies the browser cannot send
`CONNECT`, and a request whose body the browser cannot send, SHALL be unsupported.

#### Scenario: CONNECT
- **WHEN** a program writes a `CONNECT` request on the browser stream
- **THEN** the write completes with `operation_not_supported` under `ERROR`

### Requirement: HTTP: the stream's socket
`stream.socket().shutdown(...)` and `close()` SHALL end the stream: no further request is made, and they succeed. The browser stream's `socket()` MUST be a member of webcpp::beast's that offers what Beast's examples call on it: `shutdown`, `close`, `is_open`, `cancel`, `get_executor`.

#### Scenario: Shutdown after a response
- **WHEN** a program calls `stream.socket().shutdown(tcp::socket::shutdown_both, ec)` after reading a response, as `http_client_async` does
- **THEN** `ec` holds no error and no further request is made

### Requirement: HTTP: failed fetches
A `fetch` that rejects SHALL complete the read with `webcpp::beast::error::fetch_failed`; CORS and mixed content are among its causes, which the browser does not tell apart.

#### Scenario: A rejected fetch
- **WHEN** `fetch` rejects with `TypeError`
- **THEN** the read completes with `error::fetch_failed`

### Requirement: Config: the unsupported-setting macros
`WEBCPP_BEAST_UNSUPPORTED_ERROR` and `WEBCPP_BEAST_UNSUPPORTED_IGNORE` SHALL be read in `<webcpp/beast/config.hpp>`, which every public header includes first, with `WEBCPP_BEAST_NO_EXCEPTIONS`. `ERROR` MUST be defined when neither is, and both defined MUST be an `#error`.

#### Scenario: Both defined
- **WHEN** a translation unit defines both macros and includes a public header
- **THEN** compilation stops with an `#error`

#### Scenario: Neither defined
- **WHEN** a translation unit defines neither
- **THEN** `WEBCPP_BEAST_UNSUPPORTED_ERROR` is defined after `config.hpp`

### Requirement: Config: the b2 feature
A b2 feature `webcpp-beast-unsupported=error|ignore` (propagated) SHALL drive the macros from webcpp::beast's usage requirements, as `<webcpp-beast-unsupported>ignore:<define>WEBCPP_BEAST_UNSUPPORTED_IGNORE`, so that a whole program builds with one setting.

#### Scenario: A program built with ignore
- **WHEN** a program is built with `webcpp-beast-unsupported=ignore`
- **THEN** every translation unit it links is compiled with `WEBCPP_BEAST_UNSUPPORTED_IGNORE`

### Requirement: Config: the browser classes are tagged by the setting
The browser classes SHALL live in an inline namespace named after the setting, as Asio tags its own, so that translation units that disagree name different types and fail at link, never silently.

#### Scenario: Two units disagree
- **WHEN** one translation unit is compiled under `ERROR` and another under `IGNORE` and both are linked
- **THEN** linking fails with undefined or duplicate symbols naming the different inline namespaces

### Requirement: Config: the configuration test
`test/config_test.cpp` SHALL be built under each setting and with exceptions on and off, and check each case.

#### Scenario: The matrix is built
- **WHEN** the library's tests are built
- **THEN** `config_test` exists once per setting, and with exceptions on and off

### Requirement: Testing: native lanes
The native lanes SHALL test that the names are Beast's types (or derive from them) by static assertions, and that the guide's programs run against a local echo server.

#### Scenario: The guide's program runs natively
- **WHEN** the native lane runs a guide program against the local echo server
- **THEN** it exits 0 and prints the message echoed

### Requirement: Testing: the echo servers
The lanes SHALL use a native Beast echo server built by the native lane (no npm dependency), plain and TLS, because node has no WebSocket server built in. `wss` MUST use a local CA that node trusts through `NODE_EXTRA_CA_CERTS` and headless Chrome through the lane's certificate options.

#### Scenario: wss under node
- **WHEN** the Emscripten lane under node connects to the TLS echo server
- **THEN** the connection succeeds with the local CA trusted through `NODE_EXTRA_CA_CERTS`

### Requirement: Testing: Emscripten lanes under node
The Emscripten lanes SHALL run under node 26, on both suspension variants, and test: the layers (WebSocket requirements), the URL and `Host` rules, the settings under both configurations, cancellation and the destructor, back-pressure through the queue, a server that closes, refuses, or sends a message past `read_message_max`, every error of the Errors requirements that node can produce, allocation counts, and the HTTP client against the native server. They MUST NOT test CORS or mixed content, which node does not enforce.

#### Scenario: A server that closes
- **WHEN** the lane's server closes the connection with code 4001
- **THEN** the pending read completes with `websocket::error::closed` on both variants

### Requirement: Testing: the browser lane
The browser lane SHALL run the same programs in headless Chrome, and test what only a browser shows: CORS and mixed-content failures (and, with its own increment, `WebSocketStream`).

#### Scenario: A CORS failure
- **WHEN** the HTTP client in headless Chrome requests a server that sends no CORS headers
- **THEN** the read completes with `webcpp::beast::error::fetch_failed`

### Requirement: Testing: twins of the examples
Each example of the guide SHALL print the same natively and in the browser, and Beast's three examples (`websocket_client_async`, `websocket_client_async_ssl`, `http_client_async`) MUST be among them, as the examples table adapts them.

#### Scenario: A twin's output
- **WHEN** a twin runs natively and then in the browser against the same echo server
- **THEN** the two outputs are byte for byte equal

### Requirement: Examples: Beast's examples, setting by setting
For Beast 1.92's examples the twins use, the specification SHALL state what each needs under `ERROR` and under `IGNORE`. `websocket_client_async`: the decorator setting `User-Agent` fails the handshake under `ERROR` (the twin leaves it out), and runs unchanged under `IGNORE`. `websocket_client_async_ssl`: the SNI line becomes `set_server_name`; `host_name_verification`, `tlsv12_client`, `verify_peer` and `suggested(client)` are honoured; the decorator and `load_root_certificates` (added roots) fail the handshakes under `ERROR`, and the twin leaves the decorator out and calls `ctx.set_default_verify_paths()` instead; under `IGNORE` only the SNI line changes. `http_client_async`: `Host` is honoured when it is the URL's host, `User-Agent` fails the write under `ERROR` (the twin leaves it out), and `shutdown` is honoured.

#### Scenario: The SSL example under ERROR
- **WHEN** `websocket_client_async_ssl` is built on Emscripten under `ERROR` with `load_root_certificates` unchanged
- **THEN** its TLS handshake completes with `operation_not_supported`, and the twin with `set_default_verify_paths()` succeeds

### Requirement: Documentation: the page and its table of differences
The library SHALL have a documentation page with a guide, a reference, and a table of differences that names each unsupported and ignored case, and the guide's examples MUST be built and run by the tests.

#### Scenario: The page builds
- **WHEN** the documentation target is built
- **THEN** it succeeds and its table of differences lists the settings of the Settings requirements
