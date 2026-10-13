# Tasks

The specification is written; nothing else is. There is no plan and no reference code yet, so
group 0 comes first and every later task is refined by the plan. `reference/spec.md` is the
specification, `specs/beast/spec.md` its requirements, `reference/STATUS.md` says where things
stand.

## 0. Before anything: the plan and the reference code

- [x] 0.1 The specification (`reference/spec.md`, draft of 2026-10-10, with the owner's four decisions)
- [ ] 0.2 Write the implementation plan, `reference/plan.md`: every task below with its files, interfaces, steps, code and expected output, in the form of the other libraries' plans
- [ ] 0.3 Write the reference code: the library's first tree (skeleton, configuration, the native names, `set_server_name`, `teardown`) as a local branch, in the form `reference/source/` takes in the other proposals
- [ ] 0.4 Settle the open questions O1 to O3 of `design.md` with the owner and record them in the plan
- [ ] 0.5 Submit the proposal for a decision (status `proposed`) once 0.2 and 0.3 exist

## 1. The skeleton and the configuration (requirements: Library, Config)

- [ ] 1.1 The repository layout: `build.jam` (project `/webcpp/beast`, alias `beast`), `meta/libraries.json` (`"port-of": null`), `README.md`, `AGENTS.md`, licence, `doc/changelog.adoc`, CI calling the superproject's workflow
- [ ] 1.2 `<webcpp/beast/config.hpp>`: `WEBCPP_BEAST_UNSUPPORTED_ERROR` (default), `WEBCPP_BEAST_UNSUPPORTED_IGNORE`, the `#error` when both, `WEBCPP_BEAST_NO_EXCEPTIONS`
- [ ] 1.3 The b2 feature `webcpp-beast-unsupported=error|ignore` and its usage requirement
- [ ] 1.4 The inline namespace named after the setting for the browser classes
- [ ] 1.5 `test/config_test.cpp` built under each setting and with exceptions on and off

## 2. The native names (requirements: Names)

- [ ] 2.1 The aliases of Boost.Beast's types, and the network-free names as Beast's own on both targets
- [ ] 2.2 `websocket::stream` derived from Beast's with `stream(std::allocator_arg_t, const Allocator&, Args&&...)`
- [ ] 2.3 `set_server_name(ssl_stream&, std::string_view)` returning `boost::system::result<void>`, natively through OpenSSL
- [ ] 2.4 `teardown` and `async_teardown` for webcpp::asio's sockets, found by argument-dependent lookup
- [ ] 2.5 Static-assertion tests: the aliases are Beast's types, the stream derives from Beast's

## 3. The browser streams and the layers (requirements: Resolver, WebSocket layers)

- [ ] 3.1 Probe `basic_resolver_results::create` of Boost 1.92 (outside the documented interface) and pin the Boost version it needs
- [ ] 3.2 The browser `tcp_stream`: records the resolver's entry, keeps Beast's timeout interface on a Boost timer
- [ ] 3.3 `async_connect(results)` and `async_connect(endpoint)` (IPv6 in brackets, `error::no_host` on an unspecified address)
- [ ] 3.4 The browser `ssl_stream<tcp_stream>`: records the TLS settings, the scheme `wss` or `https`, and the handshake that judges them
- [ ] 3.5 `set_server_name` in the browser: succeeds, records nothing

## 4. The browser WebSocket client (requirements: WebSocket, Settings, Back-pressure)

- [ ] 4.1 `async_handshake(host, target)`: the URL, the subprotocols, `binaryType = "arraybuffer"`, the `Host` rule, completion on `open`
- [ ] 4.2 `async_read` (binary from the `ArrayBuffer`, text with `TextEncoder.encodeInto`, the view retaken after a memory growth) and `async_read_some` with `is_message_done()`
- [ ] 4.3 `async_write`, `text()`, `binary()`, `got_text()`, `got_binary()`, and `async_write_some` buffering until `fin`
- [ ] 4.4 `async_close` with the codes the browser accepts, the reason bound of 123 characters, and `read_message_max`
- [ ] 4.5 The settings table: honoured, unsupported and ignored settings, the decorator, `stream_base::timeout`, `suggested(role_type::client)`
- [ ] 4.6 The two unsupported policies at the operation that depends on the setting
- [ ] 4.7 The send bound and the receive queue, with their options, `send_limit` and `receive_limit`
- [ ] 4.8 Any completion token through `boost::asio::async_initiate`, handlers on webcpp::asio's io_context through its bridge

## 5. Cancellation, lifetime, allocation and errors (requirements: Lifetime, Allocation, Errors)

- [ ] 5.1 The cancellation table (terminal, partial, total) for handshake, read, write and close
- [ ] 5.2 The destructor: abort pending operations (posted), close without a code, unregister from the bridge
- [ ] 5.3 The operation state through the handler's allocator; the stream's own state through the given allocator
- [ ] 5.4 The category `webcpp.beast` with its seven fixed values, and the mapping of every browser outcome
- [ ] 5.5 The counting test: nothing goes outside the given allocator, on both variants

## 6. The HTTP client in the browser (requirements: HTTP; second increment)

- [ ] 6.1 The write side: parse Beast's bytes with `request_parser`, one `fetch` per whole request, `redirect "follow"`, `mode "cors"`, body buffered
- [ ] 6.2 The read side: serialize the response with Beast's serializer, chunked, with the reason phrase and without `Content-Length` and `Content-Encoding`
- [ ] 6.3 The field rules (`Host`, `Content-Length`, `Transfer-Encoding`, `Connection`, `Keep-Alive`, forbidden fields)
- [ ] 6.4 Redirects (`response.redirected`), `CONNECT` and unsendable bodies
- [ ] 6.5 `socket()` with `shutdown`, `close`, `is_open`, `cancel`, `get_executor`
- [ ] 6.6 `fetch_failed` for a rejected `fetch`

## 7. Testing (requirements: Testing)

- [ ] 7.1 Native lanes: the names, the guide's programs against a local echo server
- [ ] 7.2 The native Beast echo servers, plain and TLS, built by the native lane; the local CA for `wss` (node: `NODE_EXTRA_CA_CERTS`; headless Chrome: the lane's certificate options)
- [ ] 7.3 Emscripten lanes under node on both suspension variants, each item of the specification's section 8
- [ ] 7.4 The headless-browser lane: CORS and mixed-content failures
- [ ] 7.5 The twins of Beast's three examples, as the specification's section 4.6 adapts them

## 8. Documentation and the first release

- [ ] 8.1 The page: the guide, the table of differences naming each unsupported and ignored case, the reference
- [ ] 8.2 The guide's examples, each printing the same natively and in the browser
- [ ] 8.3 The lint, the site, CI green on the native and Emscripten lanes, the changelog line, the tag
