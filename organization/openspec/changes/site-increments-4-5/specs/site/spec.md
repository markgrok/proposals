## ADDED Requirements

### Requirement: Site: The examples gallery and its roadmap
The page `examples/` ("Examples: C++ and JavaScript, step by step") SHALL hold a gallery with one entry for every example that exists under `examples/` (the calculator, the state machine, C++ calls the browser, the collaborative cursors and on the server), each linking its live page, which runs in the visualizer, and the page that explains it, and only for examples whose page exists; its roadmap names no example that exists and still names file transfer and video chat (as trystero's roadmap delivers what they need).

#### Scenario: The gallery
- **WHEN** the examples page renders with five examples under `examples/`
- **THEN** it has five entries, each linking an existing live page and an existing explanation, and its roadmap names file transfer and video chat only

#### Scenario: The roadmap list
- **WHEN** the examples page renders with the five examples of increment 5
- **THEN** its roadmap lists two entries, file transfer and video chat, and none of the five examples

### Requirement: Site: What the site does not do after increment 5
The site as built SHALL ship no file-transfer example and no video-chat example (they wait for trystero's features), and its pages MUST say so in their roadmaps. It makes no connection on load beyond the page's own origin: no server, no runtime API, no cookie; an example connects to Trystero's public Nostr relays and to public STUN servers, and then to other readers, only when the reader presses Join, under the notice that says so; and the other third-party destination is Google's search, reached by the reader's submit on the search page. The release channel at https://webcpp.org/ is built by the same command but not yet published (it waits for the first release and the DNS move), Google's Programmable Search engine and Search Console property are the release site's, and the site's increments 6 to 8 are the roadmap.

#### Scenario: Nothing connects on load
- **WHEN** the cursors page loads
- **THEN** no request leaves the page's own origin and no WebSocket is opened before Join

#### Scenario: The reader searches
- **WHEN** a reader submits the search form
- **THEN** the browser navigates to google.com/search with the site restriction, and nothing contacted Google before

### Requirement: Trystero package: The package, its place, name and build
trystero SHALL ship a JavaScript face, the package `@webcpp/trystero`, in `libs/trystero/js/` (a directory for a library's JavaScript face, accepted by the controller as following the owner's decision on the JavaScript API, recorded in `AGENTS.md` chapter 1), built by `b2 toolset=emscripten libs/trystero/js//package` in the release variant into `<build>/package/`, whose files are `index.mjs`, `facade.mjs`, `index.d.mts`, the module's glue `trystero.js` and `trystero.wasm`, `package.json`, `README.md`, `LICENSE_1_0.txt` and `LICENSE-TRYSTERO.txt`. Its `package.json` MUST be `name` `@webcpp/trystero`, `version` `0.1.0`, `private` `true` until the owner publishes it (gate G6), `license` `BSL-1.0 AND MIT`, `type` `module` (ESM only), `exports` `{".": {"types": "./index.d.mts", "default": "./index.mjs"}}`, `sideEffects` `["./index.mjs"]` and `files` the nine names above. The module is linked with `-sMODULARIZE`, `-sEXPORT_ES6`, `-sEXPORT_NAME=createTrysteroModule`, `-sENVIRONMENT=web` (a browser's window alone: node and workers, where Trystero JS runs, are not supported, a recorded difference), `-sALLOW_MEMORY_GROWTH` and `-lembind`; `js/Jamfile` declares `alias flags` for those, `webcpp.link trystero : bindings.cpp : <library>/webcpp/trystero//trystero <library>flags : emscripten` and `make package.stamp`, which copies the files into `package/` beside the stamp, prints that directory, runs on emscripten alone and stops the build, saying so, on another toolset. `index.mjs` awaits the module at its top level, so that an `import` of the package resolves once the wasm is compiled and `joinRoom` is synchronous as Trystero's is. The package's README says what it is, the one line a Trystero page changes, what is supported and what throws, where it runs, how b2 builds it, that its files (not its surface) will change when trystero moves onto webcpp::asio, and the licences; it has no `npm install` line until gate G6.

#### Scenario: Building the package
- **WHEN** `b2 toolset=emscripten libs/trystero/js//package` runs
- **THEN** the directory printed holds the files of `package.json`'s `files`, and the size of `trystero.wasm` is recorded in the ledger

#### Scenario: Building it on another toolset
- **WHEN** `b2 libs/trystero/js//package` runs with the native toolset
- **THEN** the build stops, saying the package is built on emscripten alone

#### Scenario: The package is private
- **WHEN** `package.json` is read before gate G6
- **THEN** `private` is `true`, and no page or README of the site shows `npm install @webcpp/trystero`

### Requirement: Trystero package: raw_payload and the runtime
trystero SHALL gain, in `include/webcpp/trystero/room.hpp` (public, fully documented), `raw_payload`, an action type that hands a message's format and bytes undecoded and sends bytes with the format given: `struct raw_payload { protocol::payload_format format = protocol::payload_format::binary; pmr_bytes bytes; }` with its `payload_traits` (encoding takes the format and moves the bytes; decoding moves the frame's bytes into a `pmr_bytes` of the room's resource, with the frame's format), added to `make_action`'s `@tparam` list; without it a JSON message would be parsed in C++ and serialized again, which can change its text (`1.0` becomes `1`). `js/runtime.hpp` (the package's own, not installed, namespace `webcpp::trystero::js`) MUST provide `room_handle` (`std::uint32_t`, a number never reused by one runtime), `join_request` (views `app_id`, `room_id`, `password`, `max_receive_bytes` 256 MiB, `handshake_timeout` 10,000 ms, `relay_urls`, `relay_redundancy` 5, `ice_servers`, copied into the runtime's `std::pmr::memory_resource` at the call so that no default-heap container is made from it) and `template <class Events> class runtime` constructed from `(boost::asio::any_io_executor, backend::factory&, std::string_view self_id, Events&, std::pmr::memory_resource*)` with `self_id()`, `join` (returns `boost::system::result<room_handle>`; makes the app at its first room; `error::too_many_rooms`, `error::already_joined`), `make_action` (`error::invalid_action_name` for a bad name), `send`, `ping`, `leave` (each settles a ticket) and `peers`. It holds a `std::pmr::map` from app id to `app` (the shared connection's) and from handle to a record of the room, its actions by name and the app id; handles count from 1 and a handle is removed only after the event that leaves it returns, so that an event handler may call any member for the room it is told about, `leave` included. An unknown handle makes `make_action` return `error::room_left` and `send`, `ping` and `leave` settle with it. The runtime has no static state and no `throw`, `try` or `catch`, and each function starts with its guards. Events are `peer_join`, `peer_leave`, `message(room, action, peer, raw_payload, metadata)`, `room_error`, `app_error` and `settled(ticket, ec, value)`.

#### Scenario: Two rooms of one app share the self id
- **WHEN** runtime `a` joins `{app, "r1"}` and `{app, "r2"}` and runtime `b` joins both
- **THEN** the handles differ, `a.self_id()` is the one given, and each side records `peer_join` for both rooms with the same peer id

#### Scenario: The 65th room
- **WHEN** an app has joined 64 rooms
- **THEN** the 65th `join` returns `error::too_many_rooms`

#### Scenario: An unknown handle
- **WHEN** `send`, `ping` and `leave` are called on handle 999
- **THEN** each settles with `error::room_left`

#### Scenario: Leaving from its own event
- **WHEN** the `message()` event calls `leave(room, 9)` for the room it is told about
- **THEN** the message is recorded once, ticket 9 settles with success, a second message to that room is not delivered, and the app's other room goes on

#### Scenario: JSON is not decoded
- **WHEN** a `raw_payload` of format json carrying `{"a":1.0}` is sent
- **THEN** it arrives as the same bytes with format json at a `raw_payload` action, and as a `boost::json::value` at a `boost::json::value` action of the same name

#### Scenario: Every allocation reaches the user's resource
- **WHEN** two runtimes, each given a counting resource, join two rooms and exchange a text, a JSON and a binary payload of 100,000 bytes with metadata, ping and leave
- **THEN** every allocation goes through the given resources, and the global `operator new` and the C library's `malloc`, `calloc` and `realloc` see none between the first `join` and the last `settled`

### Requirement: Trystero package: joinRoom and selfId
The package SHALL export `selfId` (20 characters of Trystero's 62-character set, drawn once at load from `crypto.getRandomValues`, where Trystero uses `Math.random`) and `joinRoom(config, roomId, callbacks?) -> Room`. `joinRoom` MUST make one `app` per `config.appId`, at that app's first join, with `self_id` equal to `selfId`; the app's settings come from the first config of that `appId`, as Trystero's relays do (`relayConfig.urls`, `relayConfig.redundancy`, `rtcConfig.iceServers`) and the room's settings from each call (`password`, `maxReceiveBytes`, `callbacks.handshakeTimeoutMs`). A second `joinRoom` for an app and room still joined returns the same `Room`, as Trystero's does, so that the runtime is never asked twice and `error::already_joined` never reaches JavaScript. A 65th room of one app throws an `Error` with `code` `too_many_rooms` (`app::max_rooms`, 64), which Trystero lacks (a recorded difference); a refused `join` (`{code, message}`) throws an `Error` with that code. `callbacks.onJoinError({error, appId, roomId, peerId})` MUST be called, on a microtask, for a room's `error::wrong_password`, with Trystero's text. The package is built on `app` alone, so the shared connection's integration (every room of an app between two peers over one WebRTC connection) is a precondition.

#### Scenario: selfId
- **WHEN** the facade is made with a `random` that fills 7s
- **THEN** `selfId` matches `^[0-9A-Za-z]{20}$`

#### Scenario: joinRoom twice, then after leave
- **WHEN** `joinRoom({appId: 'app'}, 'r')` is called twice, the room leaves, and it is called again
- **THEN** the first two calls return the same room, the third a new one, and the module's `join` was called twice

#### Scenario: A wrong password
- **WHEN** the module reports a `roomError` of `wrong_password`
- **THEN** `onJoinError` receives `{error, appId, roomId, peerId}` on a microtask

### Requirement: Trystero package: Rooms, actions and peers
A `Room` SHALL offer `makeAction(namespace, config?)`, which returns `{send, onMessage, onReceiveProgress, onReceive}` (a second call with the same namespace returns the same action, as Trystero's does; `config.onMessage` sets the action's first handler); `ping(id) -> Promise<number>` (the milliseconds the runtime settles); `leave() -> Promise<void>`, after which `joinRoom` makes a new room; `isPassive() -> false`; `getPeers()`, a record whose keys are the joined peers and whose values are `null` (a recorded difference); and `onPeerJoin` and `onPeerLeave` as properties. Setting `onPeerJoin` MUST call it at once, synchronously inside the setter, with each peer already joined, from `peers()`, as Trystero's setter does; the replay is not deferred (the setter runs in the user's code, never inside the wasm's call, so it cannot re-enter the runtime) and a handler that throws there throws out of the setter, as in Trystero. Every room leaves on `beforeunload`, as Trystero's do.

#### Scenario: A late onPeerJoin
- **WHEN** peers `p1` and `p2` are joined and `room.onPeerJoin = id => seen.push(id)` is set
- **THEN** `seen` is `['p1', 'p2']` at once

#### Scenario: beforeunload
- **WHEN** two rooms are joined and the page fires `beforeunload`
- **THEN** the runtime receives two `leave` calls

#### Scenario: getPeers
- **WHEN** peers `p1` and `p2` are joined
- **THEN** `getPeers()` is `{p1: null, p2: null}`

### Requirement: Trystero package: Sending
`MessageAction.send(data, options?)` SHALL reject `undefined` data with Trystero's error, "action data cannot be undefined"; choose the format as Trystero's `action-wire.mjs` decides (a `Blob`, an `ArrayBuffer` or a view is binary, a `Blob` read with `arrayBuffer()`; a string is text; anything else is JSON by `JSON.stringify`, so `{a: 1}` and `42` are format 1); accept `options.target` (a string, an array or null) and `options.metadata` (JSON); and check the targets against `peers()` first: each one not joined warns `Trystero: no peer with id <id> found` and the message goes to the others, as Trystero's `iterate` does, and with none left the promise resolves. Every other failure MUST reject with an `Error` whose `message` is the C++ error's message and whose `code` is its name. The formats on the wire are text 0, JSON 1 and binary 2.

#### Scenario: Formats
- **WHEN** `send` is called with `'hi'`, `{a: 1}`, `42`, `new Uint8Array([1, 2])`, `new ArrayBuffer(3)` and `new Blob(['ab'])`
- **THEN** the runtime's `send` records formats 0, 1, 1, 2, 2 and 2

#### Scenario: A joined and an unknown target
- **WHEN** `send('hi', {target: ['p1', 'ghost']})` is called with only `p1` joined
- **THEN** the runtime sends to `['p1']`, one warning `Trystero: no peer with id ghost found` is issued, and `send('hi', {target: 'ghost'})` resolves with `undefined`

### Requirement: Trystero package: Delivery on a microtask
Every event the runtime reports SHALL be copied out of the wasm memory first (a payload's bytes with `slice()`, never the view) and then delivered to the user's handler on a microtask, never from inside the wasm's call. A handler that throws MUST be reported with `console.error('Trystero action handler error:', err)`, as Trystero's `actions.mjs` does for `onMessage` (and with the same words for the room's handlers), and the next event is still delivered, so that a handler may call `leave` or `send` at once and the runtime is never re-entered from its own event. A received text arrives as a string, JSON as `JSON.parse` of the bytes (a message whose JSON does not parse is dropped with a warning), binary as a `Uint8Array`, each with the context `{peerId, metadata}` (`metadata` parsed from JSON when present).

#### Scenario: Decoded, on a microtask
- **WHEN** the module fires a JSON message `{"a":1}` with metadata `{"m":2}` and a text message `hi`
- **THEN** the handler has been called with nothing yet, and after a tick receives `[{a: 1}, {peerId: 'peer', metadata: {m: 2}}]` and `['hi', {peerId: 'peer'}]`

#### Scenario: The cursors sketch's handler and a JSON null
- **WHEN** the handler destructures `({x, y, color}, {peerId})` and a JSON `null` arrives, then a valid cursor
- **THEN** one `console.error('Trystero action handler error:', ...)` is reported and the valid cursor is still delivered

#### Scenario: A handler that leaves
- **WHEN** an `onMessage` handler calls `room.leave()`
- **THEN** the runtime has received no `leave` until the event returns, and receives exactly one after a tick

#### Scenario: Unparseable JSON
- **WHEN** a message of format JSON carries bytes that are not JSON
- **THEN** it is dropped with a warning

### Requirement: Trystero package: Every unsupported member is refused
Every member of Trystero 0.26.0's configuration and options that the package does not implement SHALL throw a `TypeError` whose message names the member and says "is not supported by webcpp's trystero yet; see its roadmap", none accepted in silence (ignoring `rtcConfig.iceTransportPolicy: 'relay'` would expose the address it asks to hide), and a key that is in no table is refused too. The tables are: `BaseRoomConfig` supports `appId`, `password`, `maxReceiveBytes`, `passive: false`, `trickleIce: true` and refuses `passive: true`, `trickleIce: false`, `turnConfig`, `rtcPolyfill` and every `_test_only_*`; `RelayConfig` supports `urls`, `redundancy`, `manualReconnection: false`, `warnOnRelayFailure: false` and refuses `manualReconnection: true` and `warnOnRelayFailure: true`; `RTCConfiguration` supports `iceServers` and `iceTransportPolicy: 'all'` and refuses `iceTransportPolicy: 'relay'`, `bundlePolicy`, `rtcpMuxPolicy`, `certificates`, `iceCandidatePoolSize` and any other member; `JoinRoomCallbacks` supports `onJoinError` and `handshakeTimeoutMs` and refuses `onPeerHandshake`; `MessageActionConfig` supports `kind: 'message'` and `onMessage` and refuses `kind: 'request'`, `onReceiveProgress` and `onReceive`; `SendOptions` supports `target` and `metadata` and refuses `onProgress` and `signal` (as rejections, since `send` is async); a `MessageAction`'s non-null `onReceiveProgress` or `onReceive` is refused; `Room` refuses `addStream`, `removeStream`, `addTrack`, `removeTrack`, `replaceTrack` and a non-null `onPeerStream` or `onPeerTrack`; the exports refuse `getRelaySockets`, `pauseRelayReconnection`, `resumeRelayReconnection` and `defaultRelayUrls`. The defaults Trystero itself takes (`passive: false`, `trickleIce: true`, `manualReconnection: false`, `warnOnRelayFailure: false`, `iceTransportPolicy: 'all'`) are accepted.

#### Scenario: A relay-only policy
- **WHEN** `joinRoom({appId: 'a', rtcConfig: {iceTransportPolicy: 'relay'}}, 'r')` is called
- **THEN** it throws a `TypeError` whose message contains "is not supported by webcpp's trystero yet"

#### Scenario: An unknown key
- **WHEN** `joinRoom({appId: 'a', somethingElse: 1}, 'r')` is called
- **THEN** it throws a `TypeError`

#### Scenario: A refused send option
- **WHEN** `send('x', {signal})` is called
- **THEN** the promise rejects with a `TypeError`

#### Scenario: The defaults
- **WHEN** `joinRoom` is called with `passive: false`, `trickleIce: true`, `relayConfig: {urls: ['wss://x'], redundancy: 2, manualReconnection: false, warnOnRelayFailure: false}` and `rtcConfig: {iceServers: [], iceTransportPolicy: 'all'}`
- **THEN** it does not throw

### Requirement: Trystero package: Where the package cannot run
Without `crypto.subtle` (an insecure context) or without `RTCPeerConnection`, `joinRoom` SHALL throw an `Error` saying what is missing (a message that matches "secure context" or "WebRTC"), before any room is made and with no call to the module. A browser that cannot run the package (the wasm blocked, an insecure context, no WebRTC) MUST end in text: the frame says so, the visualizer says the example could not start and why, and no error reaches the console uncaught.

#### Scenario: Insecure context
- **WHEN** the facade is made with `secure: false` and `joinRoom({appId: 'app'}, 'r')` is called
- **THEN** it throws and the module has recorded no call

### Requirement: Trystero package: observe, webcpp's one addition
The package SHALL add to Trystero's surface one export, `observe(observer | null)`, where `observer = {call(name, args), result(name, value), callback(name, args)}`: `call` and `result` around every binding the facade calls into the wasm, `callback` for every event the runtime reports, with `name` the binding's (`Runtime.join`, `Runtime.send`, `events.message`, ...). Its arguments MUST be copies (strings, numbers and plain objects; a payload summarised as `{format, length}` plus, for text and JSON, its decoded value), never a view on the wasm memory and never an `ArrayBuffer` beyond the payload's own bytes. `null` removes the observer; an observer that throws is detached and the error is reported with `reportError`, never thrown into the room. Gate G4 approves `observe` by name.

#### Scenario: Copies only
- **WHEN** an observer sees a `events.message` callback of a 2-byte text `hi`
- **THEN** no ArrayBuffer in its arguments is larger than 2 bytes and the payload is `{format: 0, length: 2, value: 'hi'}`

#### Scenario: A throwing observer
- **WHEN** an observer throws
- **THEN** it is detached, `reportError` is called, and the next `joinRoom` does not throw

### Requirement: Trystero package: The embind module
The wasm module SHALL be `createTrysteroModule` with the class `Runtime`: `new Runtime(selfId, events)` (`events` = `{peerJoin, peerLeave, message, roomError, appError, settled}`), `join(request) -> {room} | {code, message}`, `makeAction(room, name) -> '' | code`, `send(room, action, format: 0|1|2, bytes: Uint8Array, targets: string[], metadata: string, ticket: number)`, `ping(room, peer, ticket)`, `leave(room, ticket)`, `peers(room) -> string[]` and `delete()`. `js/bindings.cpp` owns, in this order, the `io_context`, `browser::event_loop`, `browser::backend` and `js::runtime<val_events>` and has no static state; `val_events` turns each event into a call of the JavaScript `events` object, a payload's bytes going as a `Uint8Array` view which the facade copies before the call returns. Each binding MUST be a region `// tag::<Class.member>[]` ... `// end::<Class.member>[]` of `bindings.cpp` (such as `tag::Runtime.send[]`) and each event callback `tag::events.<name>[]`, so that the site can show them.

#### Scenario: The tags
- **WHEN** `bindings.cpp` is read
- **THEN** it holds a tagged region for each binding of `Runtime` and for each of the six events

### Requirement: Trystero package: Tests and the interop lane
The package SHALL be tested at three levels. The runtime: `test/raw_payload_test.cpp`, `test/js_runtime_test.cpp` and `test/js_runtime_allocation_test.cpp` run natively and on emscripten, with and without exceptions (the `exception-handling=off` twins declared in `test/Jamfile` as trystero's other tests are), over the memory backend. The facade: `test/js/facade_test.mjs` over `test/js/fake_module.mjs` (a stand-in whose `Runtime` records each call in `calls`, with `lastCall`, `nextCall`, `settleLast`, `fire` giving a message's bytes as a view on a fake 1 MiB `ArrayBuffer` and a replaceable `peersOf`), declared with `webcpp.js-test` in a lane `js` on emscripten that also builds the package. The package's release bytes, in trystero's interop lane: `webcpp.drive interop_api : /webcpp/trystero/js//package : <webcpp-drive-timeout>900 : interop/run.mjs : --api : emscripten ;` in the lane `interop`, with `--api` serving the package's directory at `/package/` and Chrome started with the host-resolver rule, running the scenarios `api pair with Trystero JS` (join, one message each way of each format with metadata, `onPeerJoin`, `onPeerLeave`), `api pair with a password`, `api wrong password` (`onJoinError` with the Trystero JS peer's id), `api two rooms of one app` (both rooms meet over one connection, the Trystero JS page counting one `RTCPeerConnection`), `api with the C++ peer`, `api ping`, `api leave and join again`, `api handler leaves its room` (no uncaught error, the other room goes on) and `cursors sketch` (two `cursors_page.html` pages run `cursors.js`; the pointer moves every 100 ms until the other draws it, for at most 15 seconds, the fractions checked, and closing one page removes its cursor in the other). The cursors pages cannot be given a relay without changing the sketch, so `run.mjs` adds, in the tests alone, a script replacing `window.WebSocket` with a subclass that connects to the local relay whatever URL it is given, every redundant relay socket reaching the one local relay; each message MUST be counted once on each side and each `onPeerJoin` once per peer.

#### Scenario: The facade lane
- **WHEN** `b2 -a toolset=emscripten libs/trystero/test/js//js` runs
- **THEN** the facade's tests pass and the package's directory is printed

#### Scenario: Duplicate relay events
- **WHEN** the cursors pages receive each event once per redundant relay socket
- **THEN** both sides drop the duplicates and count each message and each `onPeerJoin` once

### Requirement: Trystero package: Its page, differences and roadmap
trystero's page SHALL gain `doc/javascript.adoc`, a short chapter with no listing (the library's page is taught in C++), which says what the package is, shows `b2 toolset=emscripten libs/trystero/js//package` from the Jamfile's own words through a `tag::` region, names the surface kept and the members refused, and links the site's page "Using a webcpp library from JavaScript" as `{webcpp-site}javascript/`. `doc/differences.adoc` MUST gain a section "The JavaScript package" listing `getPeers`' values, `selfId` from `crypto.getRandomValues`, `observe`, the browser's window alone (`-sENVIRONMENT=web`), `max_rooms` and every refused member; `doc/roadmap.adoc` names the three examples of trystero.dev as the measure of a complete port (cursors runs; file transfer and video chat wait for their features); `doc/testing.adoc` names the `js` lane and `interop_api`; `README.md` names `cursors.js` (from trystero.dev) and `js/index.d.mts` (from Trystero's `types.d.mts`) as derived, with `LICENSE-TRYSTERO.txt` beside them. trystero's page builds in both layouts (`b2 -a libs/trystero/doc` and `-sWEBCPP_INDEX=site`).

#### Scenario: The page in both layouts
- **WHEN** `b2 -a libs/trystero/doc` and `b2 -a libs/trystero/doc -sWEBCPP_INDEX=site` run
- **THEN** both pass doc-check, every count computed, with `raw_payload` documented in the reference

### Requirement: Site tooling: The attribute {webcpp-site}
`tools/doc/doc.jam` SHALL set the AsciiDoc attribute `{webcpp-site}` for the libraries' pages: `../../` in the site's layout (`-sWEBCPP_INDEX=site`), where a library's page sits at `libs/<name>/`, and `https://webcpporg.github.io/` otherwise. `tools/test/doc_test.py` pins it.

#### Scenario: A link to the site's page
- **WHEN** a fixture page links `{webcpp-site}javascript/`
- **THEN** it resolves to `../../javascript/` in the site's layout and to `https://webcpporg.github.io/javascript/` in the other

### Requirement: Site tooling: webcpp.js-test
`webcpp.js-test name : files + : targets *` (superproject, `tools/drive/drive.jam`, `tools/drive/drive.py`, `tools/webcpp.jam`, named apart from asio's `/webcpp//node-test`, the C++ test support under node) SHALL be a test of b2's of the type `DRIVE`: it runs `node --test --test-timeout=60000 --test-reporter=tap <files>` with the Jamfile's `webcpp.original node` packages, through `drive.py`, which bounds it and stops its process group as for a driven test; it MUST fail when the TAP summary reports `# tests 0`, so that a lane never passes on nothing; it calls `runs-original`, so the lane is of the kind `original` and the CI sets up node for it. `AGENTS.md` chapter 9 says what each of the two is for.

#### Scenario: A passing and a failing file
- **WHEN** a fixture Jamfile declares `webcpp.original node ; webcpp.js-test pass : pass_test.mjs ;`
- **THEN** it passes, and one whose test asserts false fails naming it

#### Scenario: A file with no test
- **WHEN** a file holds no test
- **THEN** the rule fails naming "no test ran"

#### Scenario: A test that waits for ever
- **WHEN** a test never ends
- **THEN** `--test-timeout` or, past it, `<webcpp-drive-timeout>2` stops it, with no process left

#### Scenario: The lane's kind
- **WHEN** `b2 declared-lanes` lists the lane
- **THEN** its kind is `original`

### Requirement: Site tooling: The site's own lanes
A Jamfile under `site/examples/<dir>` SHALL be allowed to declare `webcpp.lane`, and `b2 declared-lanes` MUST print it as `site <lane> site/examples/<dir> <target> <kind>`, with the CI running it as a library's own lane. `webcpp.jam`'s `lane` rule accepts a Jamfile under `site/examples` as the library `site`, and `library-of` returns `site` for it, so that a lane's line and a program's record name `site`, never `site/examples/<dir>`; `rule declared` returns only the program pairs recorded under the directories it is given, so that a run which loads a site example's Jamfile first leaks no `site` pair into `declared-targets` or `libraries.json`. The Jamroot has a rule of its own, `lane-directories`, returning `library-directories` and, when `site/` is checked out, every directory of `site/examples` that holds a Jamfile; `declared-lanes` reads it and `declared-targets` keeps reading `library-directories` alone, so that the site's programs never reach `b2 declared-targets`, from which `matrix.py plan()` makes the ordinary lanes: they run only in its own lanes. `tools/lint/compile_commands.py`'s `LANE` reads `site/examples/...` too, `tools/ci/matrix.py`'s `parsed_own_lanes` accepts `site/examples(/...)` for the library `site` and refuses a directory outside it, `.github/workflows/library.yml`'s `own-lanes` job runs a lane of `site` from the superproject (its checkout already recursive) with the setup of its target (Boost for native; wasi-sdk, wasmtime, wit-bindgen and the WASI WIT for wasip2 and wasip3), and `tools/report/report.py` reads a `site` own lane's XML as a library's. The site's lane `site-test` is unchanged.

#### Scenario: A fixture lane
- **WHEN** `tools/test/fixtures/site/examples/demo/` (a Jamfile with `webcpp.targets native wasip2 ; webcpp.run demo : demo.cpp ; webcpp.lane http : demo : native wasip2 ;`) is copied to `site/examples/demo/` with an empty `site/Jamfile`
- **THEN** `b2 -d0 declared-lanes` prints `site http site/examples/demo native programs` and `site http site/examples/demo wasip2 programs`, `b2 -d0 declared-targets` prints no line that starts with `site`, and `matrix.py plan` plans no ordinary lane of `site`

#### Scenario: A lane outside site/examples
- **WHEN** the same Jamfile is built from `site/other/demo/`
- **THEN** `webcpp.lane` fails with "which is not a test or an example Jamfile of a library"

#### Scenario: A combined run
- **WHEN** `b2 declared-lanes declared-targets` runs together
- **THEN** `declared-targets` prints no `site` line

### Requirement: Site tooling: webcpp.serve-native and serve.py --native
`serve.py --native PROGRAM` (`tools/component/serve.py`, `component.jam`) SHALL serve a native program as a component is served, in place of `--wasmtime`, `--flags` and `--component`: it runs `PROGRAM --addr 127.0.0.1:0`, which MUST print `Serving HTTP on http://127.0.0.1:<port>/` on its standard error (the line wasmtime prints), and then sends the same requests, builds the same transcript, applies the same bounds and stops the program in every outcome as for a component; a program that never prints the line fails with status 2 and its standard error. `webcpp.serve-native source : requirements * : stem : targets *` is the test `<stem>_native` of b2's: it builds `source` natively and passes when its transcript of `<stem>.requests` equals `<stem>.expected`, the stem being given so that a component and a native host share one pair. `component.serve-words ( version )` returns the words `serve.py` runs wasmtime with (`serve <flags>`), factored out, for the sessions.

#### Scenario: A stand-in native program
- **WHEN** a Python script prints the `Serving HTTP` line and answers with `http.server`
- **THEN** `serve.py --native` gives the transcript a component gives

#### Scenario: A program that never prints the line
- **WHEN** the program does not print it
- **THEN** `serve.py` fails with status 2 and the program's standard error

### Requirement: Site: The package served at packages/trystero
`b2 site` SHALL build `/webcpp/trystero/js//package` for emscripten (by the mechanism the earlier increment's ledger records for building a target for emscripten under `b2 site`) and copy the package's directory into `bin/site.tmp/packages/trystero/`, with `libs/trystero/js/bindings.cpp` as `source/bindings.cpp`, before the link check; `bin/site/packages/trystero/` MUST list exactly the package's `files` and `source/bindings.cpp`, and every one is in `precache.json`. The site's `Jamfile` has a target `packages` that places the same files in `build/packages/trystero/` for `npm test`, `typecheck` and `dev`, writes `build/packages/required` and is a dependency of `test`; the `site-test` lane, which builds no emscripten, copies `index.mjs`, `facade.mjs`, `index.d.mts` and `package.json` straight from `libs/trystero/js/` into `build/packages/trystero/` and `bindings.cpp` into `build/packages/trystero/source/bindings.cpp`, without the wasm. `tools/packages.mjs` offers `packageDirectory()` (`build/packages/trystero/` when it holds a `package.json`, else null), `packageName()` (read from it), `packagesRequired()` (true when `build/packages/required` exists) and `needPackage(t)` (true when the package is there; when it is absent and not required it calls `t.skip(...)` with "the package is not in build/packages/trystero/: run npm run dev once the preview serves it, or b2 site" and returns false; when required and absent it throws, naming it), and every test that uses it starts with `if (!needPackage(t)) return`. `npm run dev` fetches the package with the examples' wasm, every file checked against the preview's `precache.json` (a file whose hash differs fails, naming it), and when the preview's `precache.json` lists no `packages/trystero/` prints "the preview serves no packages/trystero/ yet: the cursors page will say that its package is missing; b2 site-serve builds it" and goes on serving the rest.

#### Scenario: The files of the package
- **WHEN** `b2 site` has run
- **THEN** `bin/site/packages/trystero/` holds `index.mjs`, `facade.mjs`, `index.d.mts`, `trystero.js`, `trystero.wasm`, `package.json`, `README.md`, `LICENSE_1_0.txt`, `LICENSE-TRYSTERO.txt` and `source/bindings.cpp`, each listed in `precache.json`

#### Scenario: dev with a tampered file
- **WHEN** a downloaded file's hash differs from the preview's `precache.json`
- **THEN** `dev` fails naming the file

### Requirement: Site: The JavaScript path works in the site alone
`npm test` and `npm run typecheck` SHALL pass without b2: a test or a type check that needs the package skips, printing why and how to get it (`npm run dev` once the preview serves it, or `b2 site`), and never passes in silence; under b2 the same checks are required, since the `packages` target writes `build/packages/required`, and with it a missing package fails. `package.json`'s `typecheck` becomes `tsc --noEmit && node tools/typecheck.mjs`; `tools/typecheck.mjs` runs `tsc --noEmit -p tsconfig.package.json` (the package's types by `paths`, and `test/types/trystero_api.ts`, the API as Trystero's README uses it) when `packageDirectory()` is set, else prints the skip reason and exits 0, unless `packagesRequired()`, when it exits 1 naming the package. The package's name MUST be read, never typed: `test/package_name_test.mjs` checks that every non-relative `import ... from '<x>'` of `examples/**` names `packageName()` and that no page or README of the site shows `npm install <name>` while the package's `package.json` says `"private": true`. The README's JavaScript path 1 says that `npm run dev` fetches the package too and what `npm test` skips without it.

#### Scenario: npm test without the package
- **WHEN** `build/packages/` is removed and `npm test` runs
- **THEN** it passes with the package's tests reported as skipped, each with its reason

#### Scenario: Under b2 without the package
- **WHEN** `build/packages/required` exists and the package is absent
- **THEN** the tests that use `needPackage` and the type check fail, naming the package

### Requirement: Site: Sizes, measured by every build
Every `b2 site` SHALL measure, for every library that ports a JavaScript original, two sizes into `bin/site/sizes.json` (never committed): the original bundled and minified and the port's files, each raw, gzip at level 9 and brotli at quality 11, with node's `zlib`, every number a count of bytes and never typed. `node tools/sizes.mjs --measure <library>=<port's directory>:<original's npm package>:<exports> [--measure ...] --out <sizes.json>` writes `{"<library>": {"original": {name, version, exports, raw, gzip, brotli}, "port": {contains, files, raw, gzip, brotli}}}`; the original is bundled by esbuild 0.28.2 (`export {<exports>} from '<original>'`, minified, `format: 'esm'`, `platform: 'browser'`) from the dev dependency `trystero` 0.26.0 (MIT, the version and integrity of `libs/trystero/test/oracle/package-lock.json`), the port's side is every `.js`, `.mjs` and `.wasm` file of its directory (a `.d.mts`, a README or a licence is not counted), and the site's `Jamfile` writes the words (`--measure trystero=<package dir>:trystero:joinRoom,selfId`; later `--measure xstate=<dir>:xstate:createMachine,createActor`). A `--measure` whose directory holds no such file, an export the original does not export, and an export the port's `index.mjs` does not export (read with esbuild's metafile) MUST fail, naming it. `site.jam` writes the measurement into the context before the pages render (`SiteContext.sizes`), copies it to `bin/site/sizes.json` and runs `node tools/sizes.mjs --check <site dir>` after the assembly: every `<data value>` of a size in the site's pages, and every line `assemble.py` wrote under a library's title, must equal the measurement, a difference failing and naming the page, the library, the side and the field. `SizeTable()` renders the measured libraries with the three sizes of each side and what the port's side contains, each number a `<data value="<bytes>">` with its unit computed (`12.3 KB` from 12,345 bytes, 1 KB = 1000 bytes); `npm run dev` renders with `test/fixtures/sizes.json`, which the page says are a fixture's in dev only. The pages show them: a section "Size, measured" of the JavaScript door, trystero's page and the libraries page's column "Size, brotli: webcpp / original".

#### Scenario: The three sizes of a fixture
- **WHEN** a fixture port directory and a fixture original are measured
- **THEN** each side's three sizes are the byte counts of the files, of their gzip and of their brotli

#### Scenario: A number typed by hand
- **WHEN** a page shows a `<data value>` different from `sizes.json`
- **THEN** `--check` fails naming the page, the library, the side and the field

#### Scenario: An export the original lacks
- **WHEN** `--measure` names an export the original does not export
- **THEN** the tool fails naming the export

### Requirement: Site: Frames, the surfaces of live examples
An example's live surface SHALL be a page of its own whose root is `Frame(title, body)` (`src/components/frame.tsx`), shown in a page by `ExampleSurface(src, title, peers)` (`src/components/example_surface.tsx`). `Frame` renders a `<main>` without header, band or footer, `<meta name="robots" content="noindex">` on both channels, and, when the page declares imports, a `<script type="importmap">` with them, each relative to the root; a frame has no social card (`tools/cards.mjs` leaves it out), no entry in `sitemap.xml` and no line in `llms.txt` (`tools/indexing.mjs` leaves it out), and `src/server.tsx` and `src/head.ts` render it so. `ExampleSurface` renders `peers` frames of `src` side by side from 768 px and stacked below, each an `<iframe title=... loading="lazy">` with a reserved height so that the layout shift stays under 0.1, with a Join button and the notice of gate G7 above them. An example reaches the package through an import map: the surface page of the cursors imports `@webcpp/trystero` from `../../../packages/trystero/index.mjs` (relative from `examples/cursors/surface/`), with esbuild's `external` for the package's name.

#### Scenario: A surface page
- **WHEN** `examples/cursors/surface/index.html` is rendered
- **THEN** it carries `noindex`, an import map naming the package at `../../../packages/trystero/index.mjs`, and no header

#### Scenario: Not indexed
- **WHEN** the release is indexed
- **THEN** the frame is absent from `sitemap.xml` and `llms.txt`, and has no card

#### Scenario: The host page
- **WHEN** the cursors page renders
- **THEN** it holds two frames with a reserved height, the notice of G7, a Join `<button>` with an `aria-label`, and the visualizer

### Requirement: Site: The collaborative cursors
The site SHALL run Trystero's own collaborative-cursors example on webcpp::trystero compiled to WebAssembly, in `examples/cursors/` (`cursors.js`, `LICENSE-TRYSTERO.txt`, `README.md`, `draw.js`, `surface.ts`) with the pages `pages/examples/cursors.oui`, `pages/examples/cursors/explained.oui` and `pages/examples/cursors/surface.oui`. `cursors.js` MUST hold the 24-line sketch of https://trystero.dev/examples/collaborative-cursors/ byte for byte but its first line, replaced by `import {joinRoom, selfId} from '@webcpp/trystero'`: the sketch imports `{joinRoom, selfId}`, joins `{appId: 'dev.trystero.examples.cursors'}` in the room `'canvas'`, makes the action `'cursor'`, sends `{x, y, color}` as fractions of the viewport on every `window.onpointermove` and sets `cursor.onMessage` and `room.onPeerLeave`. Provenance is kept so that a wrong copy is detectable: the page fetched on 2026-10-11 (HTML SHA-256 `3d554609e6aedda46d691fdc8475f1bdcbcb86858af10312745e15ed41ecbad6`) is archived outside the site's repository with its URL, date and hash, the extraction rule (the page's one `<pre>`, its `textContent`, `\r\n` turned into `\n`, trailing line ends trimmed and one `\n` added) and the text's SHA-256 `da3d8f82a3b587106406fc783bbf3a2e334c713ec8defb10ef0d122d99480133`; `examples/cursors/README.md` records them, the one line changed, that the page states no licence, that trystero.dev's home names MIT for Trystero and that the copy relies on the owner's ruling of 2026-10-10 (every part of Trystero is MIT, the examples of trystero.dev included), and what is the site's. `test/examples_provenance_test.mjs` MUST hold `cursors.js`, with its first line replaced by `import {joinRoom, selfId} from 'trystero'`, to the recorded SHA-256, check that its first line imports `packageName()`, check the README records the hashes and the rule (the text's hash is also a constant of the test itself, so that rewriting the README's alone does not pass), and, when `../libs/trystero/test/oracle/interop/cursors.js` exists, that it is the same bytes (trystero keeps its own copy, which its interop lane runs, named in its README as derived). Trystero's notice stays beside the copy in `LICENSE-TRYSTERO.txt`.

#### Scenario: The sketch's bytes
- **WHEN** `cursors.js` has its first line replaced by the original import
- **THEN** its SHA-256 is `da3d8f82a3b587106406fc783bbf3a2e334c713ec8defb10ef0d122d99480133`

#### Scenario: trystero's copy
- **WHEN** both checkouts are present
- **THEN** the site's `cursors.js` and trystero's `test/oracle/interop/cursors.js` are the same bytes

### Requirement: Site: The helpers of the cursors and a peer's broken message
`examples/cursors/draw.js` SHALL define on `globalThis` the three helpers the sketch leaves to the application, before the sketch runs, so that the sketch needs no second change: `renderPeerCursor(peerId, x, y, color)`, `removePeerCursor(peerId)` and `colorForPeer(peerId)`. They MUST treat every peer message as hostile: positions are clamped to the surface and a non-number (`NaN`, `undefined`, a string) is not drawn; a colour is accepted only as `#rrggbb`, else the peer's own colour (`colorForPeer`, a stable valid `#rrggbb` from the id's hash), so that CSS, markup or a string of a megabyte never reaches the page; they never write HTML (`innerHTML` is never used, checked statically); at most 64 cursors are drawn; `removePeerCursor` of an unknown id does nothing. A JSON `null` makes the sketch's own handler throw, which the facade reports as Trystero does, and the next message is still delivered.

#### Scenario: Clamping
- **WHEN** `renderPeerCursor('p', 0.5, 0.25, '#ff0000')` is called, then with `x` of `-3`, `7`, `NaN`, `undefined` and `'1'`
- **THEN** the first places the element at 50% and 25%, the numbers are clamped, and the non-numbers are not drawn

#### Scenario: A hostile colour
- **WHEN** the colour is `'red; background:url(x)'`, `'<b>'` or a string of a megabyte
- **THEN** it is replaced by `colorForPeer('p')`

#### Scenario: The 65th peer
- **WHEN** a 65th distinct peer sends a cursor
- **THEN** it is not drawn

### Requirement: Site: The surface script, the notice and the gesture
`examples/cursors/surface.ts`, bundled by esbuild with the package's name `external`, SHALL wait for the parent's `{type: 'webcpp-join'}` or its own Join, then import the package and call `observe`, turning each crossing into a `TraceEvent` posted to the parent, import `draw.js` and then `cursors.js` (both copied as they are into `assets/examples/cursors/`, so that a reader sees Trystero's file as it ships). If the import fails (the wasm blocked) or `joinRoom` throws (an insecure context, no WebRTC), the frame MUST show the error's message in text and post `{type: 'webcpp-error', message}`, whereupon the visualizer says the example could not start, and why. No page connects anywhere on load: an example joins only when the reader presses Join, under the notice (for gate G7 option (a), the recommendation, to be approved with the gate): "Join connects this page to Trystero's public Nostr relays and to public STUN servers, and then directly to the other readers in this room, on this site or on any page running Trystero's own example. They see your IP address, as in any WebRTC call, and your pointer's position while you are joined. Nothing connects until you press Join." Under option (b) the app id is `org.webcpp.examples.cursors` (a second changed line, checked by the provenance test) and the notice drops "or on any page running Trystero's own example"; under option (c) the page carries no Join and no notice, and the visualizer plays a trace recorded by the two-page scenario over the local relay, committed under the site's `test/fixtures/`. The push that publishes the page carries the option the owner chose, never (a) by default. An example surface that reads the pointer MUST set `touch-action: none`, so that a finger draws instead of scrolling.

#### Scenario: Nothing before Join
- **WHEN** the cursors page loads
- **THEN** no WebSocket is opened before the button is pressed, counted by `Network.webSocketCreated`

#### Scenario: A blocked wasm
- **WHEN** `*/packages/trystero/*.wasm` is blocked
- **THEN** the surface shows the failure in text, the visualizer says it could not start, and the console holds no uncaught error

#### Scenario: A touch on a phone
- **WHEN** a finger moves in a surface at 360 px
- **THEN** the cursor moves in the other and `window.scrollY` stays 0

### Requirement: Site: The visualizer plays a surface's trace and maps crossings to C++
`Visualizer(example, js_source, cpp_source, options, frame)` SHALL, with `frame` set, load no wasm: it plays the `TraceEvent`s the named surface posts with `postMessage({type: 'webcpp-trace', event})`, from the same origin only (a message from another origin is ignored), keeps the last 256 events and replays the window it holds when the reader presses Play; `options.anchors` maps a crossing's name to a text of the JavaScript source. `src/scopes.ts` MUST export `Region {name, first, last}` (1-based lines, tags excluded), `regionsOf(source)` (every `tag::<name>[]` ... `end::<name>[]`), `scopesFrom(regions, file)` (a function from a `TraceEvent` to events: `js_to_wasm X` gives `[js_to_wasm X, cpp_enter X (file, regions[X].first)]`, `return X` gives `[cpp_exit X, return X]`, `wasm_to_js X` for an `events.*` callback gives `[cpp_enter X, wasm_to_js X, cpp_exit X]`, a name with no region gives the event alone and the build fails if the page names it) and `anchorsOf(source, anchors)` (each crossing's name to the 1-based line of the JavaScript source that holds its text, found exactly once, else the build fails naming the text: for the cursors, `Runtime.join` to `joinRoom(`, `Runtime.makeAction` to `room.makeAction(`, `Runtime.send` to `cursor.send(`, `events.message` to `cursor.onMessage =` and `events.peerLeave` to `room.onPeerLeave =`). The cursors page shows `cursors.js` in its JavaScript pane and the package's `source/bindings.cpp` in its C++ pane: the page runs the very bytes a JavaScript developer gets, and its scopes come from the crossings, each a tagged region of that file mapped by name at build time.

#### Scenario: Anchors found once
- **WHEN** `anchorsOf` runs over `cursors.js`
- **THEN** each anchor is found exactly once, and a text found twice or never fails the build

#### Scenario: A foreign origin
- **WHEN** a message `{type: 'webcpp-trace'}` comes from another origin
- **THEN** the visualizer ignores it

#### Scenario: Every named crossing has a region
- **WHEN** the package's `source/bindings.cpp` is read through `needPackage`
- **THEN** every name the cursors page lists has a region

### Requirement: Site: No public network in any build or test
No build or test SHALL reach the public network: Chrome runs with `--host-resolver-rules=MAP * ~NOTFOUND, EXCLUDE 127.0.0.1`, as the interop lane does, and every relay is `relay.mjs` of trystero's interop lane on 127.0.0.1. The browser lane MUST run a canary first: a page's `fetch('https://example.com/')` and a `new WebSocket('wss://relay.damus.io')` both fail to resolve, and if either succeeds the run stops before any other scenario. The only run that reaches a public relay is the cursors task's manual check after gate G7. The browser lane depends on `/webcpp/trystero/test/oracle//node-modules`, which brings `relay.mjs`'s `ws`.

#### Scenario: The canary
- **WHEN** the browser lane starts
- **THEN** the canary scenario runs first, and a resolved external host stops the run

### Requirement: Site: The cursors in headless Chrome
The browser lane (`test/browser/run.mjs`, `scenarios.mjs`, `/webcpp/site//browser-test`) SHALL run these scenarios for increment 4: two pages over the local relay (the lane starts `relay.mjs`, redirects the surfaces' WebSockets to it as the interop lane does, opens the cursors page twice, presses Join in both and, after `onPeerJoin` is seen in both, moves the pointer in the first page's surface every 100 ms until the second page's surfaces draw it, for at most 15 seconds, at the fractions sent; each visualizer's log pane holds, as a subsequence, `Runtime.join`, a `Runtime.send` crossing, then an `events.message` callback); the in-page pair (one page alone, its two surfaces meeting within the same bound); without the wasm; without WebRTC (`delete window.RTCPeerConnection` added by `Page.addScriptToEvaluateOnNewDocument`: Join shows "needs WebRTC" in text and the console holds no uncaught error); nothing before Join; offline (after the PWA's install the cursors page and its package load offline and Join fails in text, without an uncaught error); the `.wasm` type (every `.wasm` response, from the server and from the service worker's cache offline, has `Content-Type: application/wasm` and the console holds no Emscripten warning about streaming compilation); and a touch at 360 px (`Input.dispatchTouchEvent` moves the cursor and `window.scrollY` stays 0). `tools/serve.mjs` serves `.wasm` as `application/wasm` and the service worker's cached responses keep the type.

#### Scenario: Two pages meet
- **WHEN** both pages press Join and the pointer moves in the first
- **THEN** within 15 seconds the second draws the cursor at the sent fractions

#### Scenario: Offline
- **WHEN** the site is installed and the network is off
- **THEN** the cursors page and its package load, and Join fails in text without an uncaught error

### Requirement: Site: The state machine example
The site SHALL ship the state-machine example: the program `state-machine` (`examples/state_machine/Jamfile`), the module `StateMachine`, built from `../../app/site_app.cpp` and `../../app/bindings.cpp` with `<define>WEBCPP_SITE_TRACE` and `<include>../../include` (`webcpp_site/trace.hpp`) and the earlier increment's link flags but `-sEXPORT_NAME=StateMachine`, with the same API as `SiteApp`'s `MachineHost`; `app/site_app.cpp` gains trace scopes in `machine_host::create`, `send` and the named implementations, which compile to nothing in the site's own wasm. `MachineView(machine)` (`src/components/machine_view.tsx`) renders one tile per state of the named machine read from its JSON, the current one lit, and one `<button>` with an `aria-label` per event the machine names. The page `pages/examples/state-machine.oui` (with `state-machine/explained.oui`) takes the options `player` (the player's own machine, `machines/player.json`) and `install` (`machines/install.json`), shows the machine's JSON in the JavaScript pane and `app/site_app.cpp` in the C++ pane, shows beside the machine diagram the snapshot the C++ machine returned and the state React holds from it (a panel "The state React holds"), and explains the same JSON run here and in the site's own player. XState JS stays the oracle of the tests alone; the site runs none. `test/state_machine_test.mjs` MUST play on the built `build/examples/state-machine.mjs` a scripted sequence for each option (player: `PLAY`, `TICK`, `PAUSE`, `SPEED` 2, `PLAY`, `END`, `RESTART`; install: `MISSING`, `ACCEPT`, `PROGRESS`, `INSTALLED`, `NEW_VERSION`, `UPDATED`) and agree with XState 5.33.2 at each step in value and context; the trace of `send('{"type":"PLAY"}')` under `traced` holds, as a subsequence, `js_to_wasm send`, the C++ scope `send` with the event as argument, the scope of the implementation that assigns `played`, and `return send` with the snapshot; an event the state does not take leaves the snapshot's value unchanged, as XState does.

#### Scenario: The player's sequence
- **WHEN** the player's scripted sequence plays in the module and in XState 5.33.2
- **THEN** value and context agree at each step

#### Scenario: A refused event
- **WHEN** an event the state does not take is sent
- **THEN** the snapshot's value is unchanged

#### Scenario: The tiles
- **WHEN** `MachineView` renders `player.json`
- **THEN** there is one tile per state of the JSON, the lit tile is the snapshot's value, and every event is a `<button>` with an `aria-label`

#### Scenario: In the browser
- **WHEN** the browser lane plays the player machine's sequence by its buttons
- **THEN** the log pane ends with `done`

### Requirement: Site: xstate's size on the build that ships
The site's `sizes` target SHALL also measure xstate: `--measure xstate=<the directory of app//site-app>:xstate:createMachine,createActor`, the port being the site's own wasm as it ships (`machine_host`, its embind bindings, webcpp::xstate and Boost.JSON, untraced), so that the `sizes` target builds `app//site-app` with `<variant>release`, the variant `b2 site` ships (if the earlier increment's ledger records another variant shipped, the site's `pages` target gains `<variant>release` on its reference in the same commit, so that what is measured is what ships); its `contains` says so and the size table shows it. The libraries page shows xstate's sizes beside XState 5.33.2's. This step needs the sizes tool and waits for it; the example ships without it.

#### Scenario: xstate beside XState
- **WHEN** `b2 -a site` runs
- **THEN** the libraries page shows xstate's sizes beside XState 5.33.2's, from the build's measurement

### Requirement: Site: On the server, one handler and two mains
The example `examples/on_the_server/` SHALL serve one handler as a WASI component under wasmtime (wasip2 and wasip3) and natively: `answer.hpp` has `answer(method, target) -> webcpp::wasi::http::response`, `calculator_http.cpp` (named after its transcript) is the component, `WEBCPP_WASI_HTTP_MAIN_BEGIN(method, target) return answer(method, target); WEBCPP_WASI_HTTP_MAIN_END()`, and `native_host.cpp` is the native host of the example's own on Boost.Beast (synchronous, one connection at a time), since webcpp::wasi has no native host (a host moves to webcpp::beast when that library exists). `answer` decodes a query with Boost.URL (`parse_origin_form`, `params()`) and evaluates it with pratt through the calculator's `evaluate`: `GET /?expr=<percent-encoded>` answers 200 `text/plain` with the value and a newline; a `GET` whose expression does not parse answers 400 `text/plain` with pratt's message and a newline; a `GET` whose query has no `expr`, or more than 4096 bytes in it, answers 400 saying which; a query that does not decode to UTF-8 answers 400; a path other than `/` answers 404; any other method answers 405, all `text/plain`. `native_host --addr 127.0.0.1:<port>` (0 for any) MUST print `Serving HTTP on ...` on standard error, answer chunked so that its transcript holds, as wasmtime's does, no `content-length`, refuse a header block over 8 KiB (Beast's `header_limit`) with 431 and a body of more than 0 bytes with 400 (`Content-Length: 0`, which `http.client` sends with a `POST`, is no body, so that such a `POST` reaches `answer` and gets its 405). One transcript, `calculator_http.requests` with `calculator_http.expected` (`GET /?expr=1%2B2`, `1%2F3*3`, `2%2B`, `%FF`, an `expr` of 4,097 digits, one of 6,000 digits, `GET /nowhere`, `POST /`), is checked by `webcpp.serve calculator_http.cpp` (the test `calculator_http`, wasip2 and wasip3) and by `webcpp.serve-native native_host.cpp : ... : calculator_http` (the test `calculator_http_native`, native), together with `answer_test.cpp` (`webcpp.run`, native, wasip2 and wasip3), in the site's own lane `http`: `webcpp.lane http : answer calculator_http calculator_http_native : native wasip2 wasip3 ;`. The transcript is reviewed as the evidence it is: the code is fixed, never the transcript, until the three agree. Boost.URL comes from `<library>/webcpp//boost_url` when the superproject's `main` has it, else from `boost_url.cpp` (`#include <boost/url/src.hpp>` alone), a static library every program of the Jamfile links, removed by the change that brings `/webcpp//boost_url`.

#### Scenario: The answers
- **WHEN** `answer` is called with `1/3*3`, `2^10`, an `expr` of 4,097, 6,000 and 100 KB bytes, `%FF%FE` and a `POST`
- **THEN** it answers `1`, `1024`, 400, 400, 400, 400 and 405

#### Scenario: The same transcript three times
- **WHEN** the lane `http` runs on native, wasip2 and wasip3
- **THEN** the same `calculator_http.expected` holds on all three, and `answer` passes on all three

#### Scenario: A header block over 8 KiB
- **WHEN** the native host receives one
- **THEN** it answers 431, and the 6,000-byte query below its limit gets the same 400 and text from wasmtime and from the host

### Requirement: Site: Recorded terminal sessions
`tools/sessions.mjs --transcript <expected> --serve "<words>" --title "<title>" --out <session.json>` SHALL write into the build directory (`<build>/sessions/on-the-server.json`, read by `build.mjs --render --sessions <dir>`, nothing written into the source tree) `{title, elided, steps}`: `elided` names the host headers the transcript leaves out (`date`, `transfer-encoding`), which the page says; the first step's command is the words `serve-words` gives, with `--addr 127.0.0.1:8080` and the component's name (for the native host, `./native_host --addr 127.0.0.1:8080`), and its output the line wasmtime prints for that address, `Serving HTTP on http://127.0.0.1:8080/`; each later step is a `$ ` line of the transcript, as `serve.py` wrote it, with what follows it. A transcript with no `$ ` line fails, naming the file. `b2 site` writes three sessions (wasip2 under wasmtime, native, and a third, the build commands, from the Jamfile's `tag::` regions) from the committed transcript, since `b2 site` has neither wasi-sdk nor wasmtime and no output is typed. `Terminal(sessions)` (`src/components/terminal.tsx`) replays each session one character at a time from the reader's Play, with Pause and Skip as `<button>`s with `aria-label`s and a line under each saying which headers were elided; under `prefers-reduced-motion` it shows whole; it is text in a `<pre>` from the first paint, so that a crawler reads the session.

#### Scenario: Session steps are the transcript's
- **WHEN** a fixture transcript is turned into a session
- **THEN** the steps are its commands and outputs byte for byte, and nothing is written outside `--out`

#### Scenario: A crawler reads the session
- **WHEN** the server render of `Terminal` is read
- **THEN** it holds every command and output as text and the elided headers' line

### Requirement: Site: The on-the-server page
The page `pages/examples/on-the-server.oui` (with `on-the-server/explained.oui`) SHALL show the three places side by side from 1280 px and as tabs on a phone: the browser's leg is the calculator, live, from the earlier increment's wasm; the WASI leg and the native leg are the sessions; its C++ pane shows `answer.hpp` with the two mains. The explanation MUST say one handler and two mains, why the transcripts are the same and which host headers they leave out, how to run each with the commands from the Jamfile, wasmtime 47 as the prerequisite, and that the native host moves to webcpp::beast when that library exists. The browser lane's scenario loads the page, replays the sessions to their end on Play and sees the calculator leg answer `1+2` with `3`.

#### Scenario: The calculator leg
- **WHEN** the reader enters `1+2` in the browser's leg
- **THEN** it answers `3`

### Requirement: Site: C++ calls the browser
The example `examples/cpp_calls_browser/` SHALL be a program on webcpp::asio, last of the increments, scheduled after the asio track's publication (`libs/asio` a submodule of the superproject's `main`; its Tasks 1 to 8, 12, 13, 17 and 18): `webcpp.variants guess : guess.cpp : <library>/webcpp/asio//asio <include>../../include <define>WEBCPP_SITE_TRACE ;` writes `guess.variants.json` and both variants (JSPI and Asyncify, a browser loading one, asio's loader `load(manifestUrl, options)` choosing), both landing in `bin/site/examples/cpp-calls-the-browser/` and both precached. `main` builds an `io_context`, takes `emscripten::val::module_property("page")`, `co_spawn`s `play`, `clock` and `stop_on`, runs the `io_context` (`// lint-run: the page's event loop, suspended into the browser's`) and then calls `page.call<void>("ended")`, because under Asyncify a `main` that suspended loses its return value and node exits 0 once no handle is left: the page is told positively that `run` returned. The page exports to C++ (`page.ts`, each wrapped by `traced` as the module's imports): `pick(low, high) -> number` (a number from `crypto.getRandomValues`), `say(text)`, `ask(question) -> Promise<number>` (resolved by the form's submit, rejected by Give up), `tick(seconds)`, `newGame` (an `EventTarget` C++ awaits with `async_wait_event(..., "click")`), `stop` (an `EventTarget` the page dispatches `stop` on when it leaves, `pagehide`, or a test ends the run) and `ended()`. `play` picks, then asks in a loop, answering "Higher." or "Lower." by `say`, and after a win or a rejection awaits `newGame`'s click and plays again; `clock` ticks a `steady_timer` each second; `stop_on` awaits `stop`'s event, cancels `play`'s and `clock`'s pending operations through a cancellation signal both are bound to, and calls `io.stop()`, so that `run` returns and `main` ends. Every `co_await` uses `as_tuple(use_awaitable)`, so that nothing throws on either variant. The trace scopes are `play`, `round` (with the guess and the comparison's result) and `clock`; the trace shows `pick`'s return, the secret, as it shows everything that crosses the bridge, and the page says so.

#### Scenario: A full game
- **WHEN** `pick` returns 42 and `ask` answers 50, 25 and 42
- **THEN** `say` receives "Lower." and "Higher." in turn, then the win with 3 tries, and the trace holds, as a subsequence, `wasm_to_js ask`, the scope `round` with 50, then `wasm_to_js say ["Lower."]`

#### Scenario: Give up
- **WHEN** the `ask` promise rejects
- **THEN** `say` receives the farewell, the program awaits `newGame`, and a dispatched `click` starts a new game (`pick` called again)

#### Scenario: Stopped while waiting
- **WHEN** a promise is left pending and `stop` is dispatched
- **THEN** the `ended` crossing appears within 5 seconds, the child exits 0 (it sets `process.exitCode = 1` at start and 0 only in its `ended` import), no handle is left, and standard error holds no error

#### Scenario: Both variants
- **WHEN** `guess_test.mjs` runs each variant in a child process under asio's loader
- **THEN** JSPI passes where node has `WebAssembly.Suspending`, Asyncify always, each child is killed with its process group when its test ends, and each test is bounded by `node --test --test-timeout=60000`

#### Scenario: Asyncify chosen by the loader
- **WHEN** the browser lane deletes `WebAssembly.Suspending` with `Page.addScriptToEvaluateOnNewDocument`
- **THEN** the loader picks Asyncify, the page says "Variant: Asyncify", and the game plays

### Requirement: Site: Every example has its page and the gallery lists only those
Every example under `examples/` SHALL have a live page and a page that explains it, with a gallery entry on the examples page linking both, and the gallery MUST list only examples whose page exists. `test/pages_test.mjs` checks that every example that exists has a gallery entry linking its live page and its explanation, that every gallery entry links a page that exists, that the roadmaps of `examples.oui` and `javascript.oui` name no example that exists and still name file transfer and video chat, and that the JavaScript door has the four sections of the specification's section 5 (using a webcpp library from JavaScript; exporting JavaScript functions to C++; memory, speed and size, measured; running the same code on a server), each linking a page that exists. The home's live example above the fold stays the calculator. The explanation pages are `examples/<name>/explained/`; the live pages `examples/cursors/`, `examples/state-machine/`, `examples/on-the-server/` and `examples/cpp-calls-the-browser/`; the JavaScript door also gains "Using a webcpp library from JavaScript" (showing the import map and the import line of the example, read from the files), "Exporting JavaScript functions to C++" and "Running the same code on a server", and the C++ door "Building for each target".

#### Scenario: A gallery entry for a missing page
- **WHEN** an entry links a page that does not exist
- **THEN** `pages_test` fails

#### Scenario: A new example without an entry
- **WHEN** a directory is added under `examples/` without a gallery entry
- **THEN** `pages_test` fails

### Requirement: Site: The example pages answer queries
`seo/queries.json` SHALL gain, as each example lands, the queries its page answers, each checked by the SEO test (its words in the page's title, `<h1>` or description): "C++ WebRTC", "C++ peer to peer browser" and "use C++ from JavaScript" answered by the cursors page or the JavaScript door; "C++ state machine XState" by the state-machine page; "WASI C++", "C++ wasmtime" and "WebAssembly containers C++" by the on-the-server page or the C++ door; and "call JavaScript from C++" by the C++-calls-the-browser page or the JavaScript door.

#### Scenario: A query the page never says
- **WHEN** an example's page lacks a word of a query assigned to it
- **THEN** `seo_test` fails naming the query and the word

### Requirement: Site: Mobile first for examples
Every new page of the increments SHALL be checked at 360, 768, 1280 and 1440 px, light and dark, with reduced motion and with sound where an example has it, and at 360 px with touch emulation for the cursors, and every fault is fixed before the commit; the keyboard alone must work the state-machine and C++-calls-the-browser pages.

#### Scenario: The four widths
- **WHEN** the cursors page is opened at 360, 768, 1280 and 1440 px
- **THEN** it shows no horizontal overflow outside code and tables, and the two surfaces are stacked below 768 px

### Requirement: Site: The owner's gates of the increments
These outward acts and decisions SHALL be the owner's, resolved by the controller before the task that needs them and never performed by a subagent: G2, the push of trystero, the site and the superproject after each increment's review; G4, trystero's JavaScript API (recommendation: approved in advance, to stand once its independent review is clean, naming `observe`, the refusal list and the microtask delivery); G5, decided on 2026-10-11, the package is `@webcpp/trystero`, version `0.1.0`, `"private": true` until G6 (creating the npm organisation `webcpp` is the owner's own act); G6, publication on npm (recommendation: only from CI, with npm provenance, under the organisation enforcing two-factor authentication, after the package has run on the preview through one review cycle; publication runs through a b2 target only; until then the JavaScript door shows the package as served by the site and built by b2); G7, the public network from the site (options (a), (b), (c) of the requirement "Site: The surface script, the notice and the gesture"; resolved before Task 28's last step and before the push that publishes the page); and O5, the sizes (recommendation: measured by every build, never a committed file; if the owner keeps a committed file instead, a superproject commit that moves a measured library regenerates the site's file in the same commit, the package's flags gain `-DBOOST_DISABLE_CURRENT_LOCATION` and a prefix map for Boost's root, and determinism is proven between macOS and the CI's Linux). Settled by the controller on 2026-10-11: no host in webcpp::wasi for now (a native host comes with webcpp::beast), no version of C++-calls-the-browser on `emscripten::val` alone, the owner's ruling of 2026-10-10 on Trystero's licence stands, and `libs/trystero/js/` is accepted.

#### Scenario: The push that publishes the cursors
- **WHEN** the cursors page is about to be published
- **THEN** G7 is resolved and the push carries the option chosen

#### Scenario: Publishing to npm
- **WHEN** `npm publish` of `@webcpp/trystero` is considered before G6
- **THEN** it is refused: the package is private and a page test refuses `npm install @webcpp/trystero` text


## MODIFIED Requirements

### Requirement: Site: What b2 site does
`b2 site` SHALL run these steps in order, each failing the build with its faults named: (1) `site-declared.txt`, the `<library> <target>` pairs the libraries declare (`webcpp.declared`, as `declared-targets` prints them); (2) `libraries.json`, the catalogue, written by `tools/site/catalogue.py`; (3) the package, `/webcpp/trystero/js//package` built for emscripten, and the sizes, measured by `tools/sizes.mjs` into the build directory; (4) the site's `context.json` (the channel, the superproject's commit and its date, the version or null, the catalogue and the sizes) by `write-context`; (5) the site's pages (`/webcpp/site//pages`: `node-modules`, the bundles, the pages rendered with their cards, assets, icons, the client bundle, the examples' scripts and the sessions `npm run build -- --sessions` writes from the committed transcripts, and the two headers `docs-header.html` and `site-header.html`); (6) `tools/ci/assemble.py` lays the site out in `bin/site.tmp/`, with the package under `packages/trystero/`, `sizes.json` and, under each measured library's title, its size line (`--sizes`); (7) the indexing files (`npm run build -- --index`); (8) `assemble.py --check` checks every link; (9) `node tools/sizes.mjs --check` holds every size a page shows to the measurement; (10) the build is moved into a directory of `bin/site.builds/` and the link `bin/site` is swapped onto it in one rename; (11) `/webcpp/site//valid-html` checks every page of `bin/site/`. Every step writes `bin/site.tmp`, never `bin/site`, so a failed step leaves `bin/site` as it was, the previous build or none.

#### Scenario: A failed step
- **WHEN** a page fails to render during `b2 site`
- **THEN** `bin/site` still points to the last build that passed every step, or does not exist

#### Scenario: A deleted page
- **WHEN** a page is deleted from `pages/`
- **THEN** the next `b2 site` has no such page, because the rendered pages are removed first

#### Scenario: A size typed on a page
- **WHEN** a page shows a size that differs from the build's measurement
- **THEN** the step (9) fails the build and `bin/site` stays the previous build

### Requirement: Site: The layout of bin/site
`bin/site/` SHALL hold: `index.html` (the home, from `pages/index.oui`); `libraries/`, `cpp/`, `javascript/`, `ai/`, `examples/`, `community/` and `search/` (each from its `pages/<name>.oui`); the pages of the examples, `examples/<name>/` and `examples/<name>/explained/` from `pages/examples/`, with the surface of the cursors at `examples/cursors/surface/`, and the scripts of an example under `assets/examples/<name>/`; `packages/trystero/` (the package's files and `source/bindings.cpp`); `assets/` (the styles, the logo, the icons and the client bundle `site.js`); `cards/` (each page's and each library's social card, 1200 by 630); `favicon.ico`; `docs/` (the index page, built with `-sWEBCPP_INDEX=site`); `libs/<name>/` (each library's page); `report/` (the test matrix); `robots.txt`; `sitemap.xml` on the release alone; `llms.txt`; `sizes.json` (the build's measurement) and `libraries.json` (the catalogue). A place the site's own pages already hold (`docs/`, `libs/`, `report/`) fails the assembly.

#### Scenario: The site's directories
- **WHEN** a preview is built with five libraries
- **THEN** `bin/site/libs/` holds five directories, `bin/site/docs/index.html` exists, and `bin/site/sitemap.xml` does not

#### Scenario: The package and the sizes
- **WHEN** a preview is built
- **THEN** `bin/site/packages/trystero/index.mjs` and `bin/site/sizes.json` exist

### Requirement: Site: The JavaScript door
The page `javascript/` ("Use C++ from JavaScript, in the browser") SHALL list, with `LibraryList("emscripten")`, exactly the libraries that build for emscripten and no other, point to the chat page of trystero's repository (a room that is C++ compiled to WebAssembly meeting its peers over public Nostr relays), and four sections, each linking a page that exists: "Using a webcpp library from JavaScript" (the import map and the import line of the cursors example, read from the files, and trystero's `@webcpp/trystero` as served by the site and built by b2, with no `npm install` line while the package is private), "Exporting JavaScript functions to C++", "Size, measured" (`SizeTable()`, each library's WebAssembly beside its JavaScript original's, raw, gzip and brotli) and "Running the same code on a server"; its roadmap names no example that exists and still names file transfer and video chat.

#### Scenario: Only emscripten libraries
- **WHEN** the catalogue has libraries with and without the target `emscripten`
- **THEN** the page's `<main>` links `../<page>` for a library exactly when its targets include `emscripten`

#### Scenario: The four sections
- **WHEN** the JavaScript door renders
- **THEN** it has the four sections above, each linking a page that exists

### Requirement: Site: The pages of the site
The site SHALL have these eight pages of `pages/`: the home (`''`, "Where C++ and the web meet", the hero with links "See the libraries" and "Get started", the three places the same code runs, the three doors, and an examples section), `libraries/`, `cpp/`, `javascript/`, `ai/`, `examples/`, `community/` and `search/`, and, for each example, its live page and its explanation (`pages/examples/<name>.oui` and `pages/examples/<name>/explained.oui`), and the surface of the cursors (`pages/examples/cursors/surface.oui`), a frame. The navigation in the header MUST be `Libraries`, `C++`, `JavaScript`, `AI`, `Examples`, `Report`, `Community` and `Search`, each relative to the page, the current one with `aria-current="page"`, folding under a `<details class="menu">` Menu on a phone with no script. The footer says webcpp is free software under the Boost Software License 1.0 with its source on GitHub. A page has one `<h1>`: its Hero's title when its first block is a Hero, else its own.

#### Scenario: The eight pages
- **WHEN** the top level of `pages/` is listed
- **THEN** the pages are `''`, `ai/`, `community/`, `cpp/`, `examples/`, `javascript/`, `libraries/` and `search/`

#### Scenario: The header from a deep page
- **WHEN** the page at `libraries/` renders
- **THEN** its header links `../libraries/`, `../cpp/`, `../javascript/` and `../ai/`

### Requirement: Site: Pages are OpenUI Lang rendered to static HTML
Each page SHALL be written in OpenUI Lang under `pages/` (`index.oui` is the home, `<name>.oui` the page `<name>/`) and rendered at build time by the same renderer and components the browser uses (`renderToString`), into `<out>/<path>/index.html`, then hydrated in the browser. Each document MUST be a shell of four elements in this order and nothing else inside `<body>`: `<noscript>` (one message saying the site needs JavaScript), `<div id="root">` (OpenUI's rendered tree: a Page's header, band, `<main>` and footer; a Frame's `<main>` alone), `<script type="text/openui" id="page-source">` (the page's OpenUI Lang source, which cannot close its script element) and `<script type="application/json" id="site-context">`. What `#root` holds equals what OpenUI renders from the carried source and context. A page that names an unknown component fails with `<file>:<line>: <message>` and exit 1; a link from the server's root is refused; the pages' markup is their components': no string of the source holds a tag. The components, each with its zod props, are `Page` (root), `Header`, `Band`, `Footer` (the Page's own, which no block may hold), `Hero`, `Section`, `Text`, `Code`, `Link`, `Doors`, `Targets`, `LibraryTable`, `LibraryList`, `Roadmap` and `SearchForm`, those the earlier increments add (the visualizer among them) and, from these increments, `Frame`, `ExampleSurface`, `Terminal`, `SizeTable` and `MachineView`.

#### Scenario: An unknown component
- **WHEN** `pages/bad.oui` is `root = Page("x", "y", [a])` and `a = Nope("z")` on line 2
- **THEN** rendering throws a PageError whose file is `pages/bad.oui` and line 2, naming `Nope`

#### Scenario: The shell
- **WHEN** `ai/index.html` is read
- **THEN** its body is the noscript message, `<div id="root">...</div>`, the page source script and the site context script, in that order

### Requirement: Site: The libraries page and the catalogue
The libraries page SHALL list every library of the catalogue in a table as Boost lists its own, with the columns Library (linked to its page), Description, Category, Targets and Original (the original a port follows with its version, linked, or "webcpp's own"), and "Size, brotli: webcpp / original" (the build's measurement, each number a `<data value>` of bytes with its unit computed, empty for a library that ports no JavaScript original), a stacked layout with each cell's `data-label` on a phone, and a section "Bring a library" that links `community/`. The catalogue, `libraries.json`, MUST be written by `tools/site/catalogue.py --libs <libs-dir> --declared <file> --out <libraries.json>` from every library's `meta/libraries.json` (read as `tools/doc/libraries.py` reads them) and the targets the build declares: a JSON array of objects `{key, name, description, category, targets, port_of, page, repository}` sorted by `key`, with `indent=4`, `sort_keys=True`, `ensure_ascii=False` and a final newline; `key` must be the directory's name; `port_of` is null or `{name, language, version, url, licence}`; `page` is `libs/<key>/` and `repository` `https://github.com/webcpporg/<key>`; a library's `targets` are the declared lines' targets for it, sorted and without duplicates; on an error it writes nothing, prints `catalogue.py: <file>: <what>` and exits 1.

#### Scenario: One row per library
- **WHEN** the catalogue holds five libraries
- **THEN** the libraries page has five `<tr>` in its `<tbody>`, each with `<a href="../libs/<key>/">`

#### Scenario: A key that is not the directory
- **WHEN** `libs/xactor/meta/libraries.json` has key `actors`
- **THEN** `catalogue.py` exits 1 naming the file

#### Scenario: A measured library
- **WHEN** the measurement holds trystero
- **THEN** its row shows its brotli size beside Trystero 0.26.0's, from `sizes.json`

### Requirement: Site: The libraries' pages in the site
`tools/ci/assemble.py --pages DIR --docs ROOT --report DIR|- --chrome FILE --site-chrome FILE --catalogue FILE --base URL --channel preview|release --out DIR` SHALL lay out the site in DIR (empty or nonexistent): the site's own pages with `assets/` and `cards/`, `docs/` (what `ROOT/doc/html` holds), `libs/<name>/` (what `ROOT/libs/<name>/doc/html` holds) and `report/` (the test matrix, or with `--report -` one page saying no results were given); with `--sizes <sizes.json>`, a library page that has an entry gains, right after its `<h1>`, one `<p class="webcpp-size">` with `<data>` values, and a page without one is unchanged. Every page of `docs/` and `libs/` and the report's page of the second kind gets the site's header inserted after its `<body>` with each relative link rebased from the page and `<link rel="stylesheet">` to `assets/chrome.css` before its `</head>`; Asciidoctor's pages, light alone, get `--chrome` (the light mark), the report's page of the second kind (the site's own style, turning dark with the reader's theme) gets `--site-chrome`. A library's page gets the head the site's pages have, from the catalogue: the page's own title `<name>: <subject>` as Asciidoctor wrote it, then " - webcpp", at most 60 characters (a page whose title does not start with "<name>: ", or is longer, fails naming the library); the catalogue's description; canonical, Open Graph and Twitter card with the image `cards/libs/<key>.png`; and JSON-LD `SoftwareSourceCode` (`isBasedOn` the original of a port), `TechArticle` and `BreadcrumbList`. Exit 0 with the site written; 1 when an input is missing or disagrees or a page's robots does not agree with its channel, each fault named; 2 on a usage error or when DIR exists and is not empty.

#### Scenario: A library title too long
- **WHEN** a library's page title with " - webcpp" exceeds 60 characters
- **THEN** `assemble.py` exits 1 naming the library

#### Scenario: Rebased header links
- **WHEN** the site's header is inserted into `libs/xactor/index.html`
- **THEN** its links read `../../libraries/` and so on, relative from the page

### Requirement: Site: The head of every page
Every page's head SHALL be written by the shell from the page's `Page(title, description, ...)` and the catalogue: `<html lang="en">`; `<title>` of at most 60 characters, the page's subject first, then " - webcpp"; `<meta name="description">` of 120 to 160 characters (a page outside the limits fails with its file and a message naming the description); `<link rel="canonical">`, absolute by channel (`https://webcpporg.github.io/<path>` or `https://webcpp.org/<path>`); one `<meta name="robots">`; Open Graph (`og:title`, `og:description`, `og:url`, `og:type` website, `og:site_name` webcpp, `og:image` with `og:image:width` 1200, `og:image:height` 630 and `og:image:alt`); `twitter:card` `summary_large_image` with its title, description, image and alt; and one JSON-LD `@graph` with `WebSite` (with a `SearchAction` to `search/?q={search_term_string}`), `Organization` (webcpp, `logo` the site's `assets/logo.svg`, `sameAs` `https://github.com/webcpporg`), `WebPage` and `BreadcrumbList` from the page's path; on the libraries page also an `ItemList` of `SoftwareSourceCode` (one per library: `programmingLanguage` "C++", `codeRepository`, `license` `https://www.boost.org/LICENSE_1_0.txt`, `runtimePlatform` from its targets in the order WebAssembly, Emscripten, WASI 0.2, WASI 0.3, native, `isBasedOn` the original of a port). A page whose root is a Frame (an example's surface) writes `noindex` whatever the channel and no social card. A page has one `<h1>`, headings in order and every image an `alt`. `<meta name="keywords">` is not written.

#### Scenario: A short description
- **WHEN** a page's description is "Too short."
- **THEN** rendering fails with a PageError whose file is the page and whose message mentions the description

#### Scenario: The graph of an ordinary page
- **WHEN** any page but the libraries page is rendered
- **THEN** its graph holds `BreadcrumbList`, `Organization`, `WebPage` and `WebSite` and nothing else

### Requirement: Site: The preview is not indexed
Every page of the preview channel SHALL carry exactly one robots meta, `<meta name="robots" content="noindex">`, because it is the development version (owner, 2026-10-11), and the release's exactly one, `index, follow`, never `noindex`, but for a Frame, the surface of a live example, which carries `noindex` on both channels; the preview writes no `sitemap.xml`, and its `robots.txt` still lets every crawler in and names no sitemap, so that a crawler reads each page's noindex. Every page `assemble.py` lays out (of `docs/`, `libs/` and `report/`, the report's pages as `report.py` wrote them included) MUST get `<meta name="robots" content="noindex">` in its head on the preview, and then every page of the site, the site's own too, is checked: on the preview each must carry noindex and nothing else of robots, on the release none may carry noindex, and each page that does not hold is named. Google's search and Search Console belong to webcpp.org, not the preview.

#### Scenario: A preview page
- **WHEN** any page of a preview build is read
- **THEN** its only robots meta is `noindex`

#### Scenario: A library page on the release
- **WHEN** a library's page is laid out for the release
- **THEN** its robots meta is `index, follow`

### Requirement: Site: The indexing files
After the site is laid out, `tools/indexing.mjs --site <out> --base <url> --catalogue <libraries.json> --channel <preview|release> [--superproject <dir>]` SHALL write into `<out>`: `robots.txt` (`User-agent: *` / `Allow: /`, then `User-agent: Google-Extended` / `Allow: /`, and on the release `Sitemap: <base>sitemap.xml`); `sitemap.xml` on the release alone (every `<out>/**/index.html` but a Frame's as `<base><path>`, sorted, each with `<lastmod>` the date, `YYYY-MM-DD`, of the last commit that changed its source: the `.oui` under the site's `pages/`, with `--superproject` a library's repository for its page and `doc/` for `docs/`; a page with no known or committed source has none); `llms.txt` (`# webcpp`, a quote of the home's description, `## Libraries` with a line per library, `## Pages` with a line per other page but a Frame's, with its title and description, `## Data` with `libraries.json`); and `libraries.json` (the catalogue copied as it is). `<base>` is the channel's absolute URL and MUST end with `/`, the one place the site's URLs are absolute; a base without its final slash and a channel other than preview or release fail, naming the option. The same options are those of `build.mjs --index`.

#### Scenario: The release's robots.txt
- **WHEN** the release is indexed with base `https://webcpp.org/`
- **THEN** `robots.txt` allows every crawler, names `Google-Extended`, and ends with `Sitemap: https://webcpp.org/sitemap.xml`

#### Scenario: A base without a slash
- **WHEN** `--base https://webcpp.org` is given
- **THEN** the script exits 2 saying the base is not an absolute http(s) URL ending with "/"

### Requirement: Site: Social cards, icons and the logo
The build SHALL render a social card per page (a Frame has none) and per library (`cards/<path or home>.png`, `cards/libs/<key>.png`, 1200 by 630) from `assets/card.svg` with the logo's drawing and the page's title set in Inter (SIL OFL 1.1, `@expo-google-fonts/inter`, pinned by the lockfile and loaded alone, `loadSystemFonts: false`) by `@resvg/resvg-js`, so that a card is the same bytes on every machine; it renders the icons from `assets/logo.svg` (`assets/icons/icon-accent-192.png` and `-512.png`, `apple-touch-icon.png` at 180 px, `favicon.ico` with PNGs at 16, 32 and 48 px, and `assets/logo-light.svg`, the mark in the light theme's colours alone), so that no image is drawn twice. The logo is the owner's choice of 2026-10-11 (concept b2b): the globe in ink with the braces' arms, the `< >` in the accent, the accent violet (`#5b3fd9` light, `#9d8cff` dark); on the app icons' accent ground the angle brackets are black (3 to 1 at least against the ground, since the near-black ink would give 2.53). The app icons are square and full-bleed with the mark inside a maskable icon's safe zone (80%).

#### Scenario: A card
- **WHEN** `cards/cpp.png` is read
- **THEN** it is a PNG whose IHDR says 1200 by 630, identical bytes on every render

#### Scenario: Contrast of the mark on the accent
- **WHEN** the app icon is drawn
- **THEN** the angle brackets are black on the accent ground

## REMOVED Requirements

### Requirement: Site: The examples page
**Reason**: Increments 4 and 5 make the examples page a gallery of five live examples, and its roadmap no longer lists them; the requirement is replaced, under its new name, so that its scenarios describe the gallery rather than the page of increment 3.
**Migration**: See "Site: The examples gallery and its roadmap": the gallery lists every example under `examples/` with its live page and its explanation, and the roadmap names only file transfer and video chat.

### Requirement: Site: What the site does not do yet
**Reason**: The site gains its PWA (a service worker, its manifest and `precache.json`, written last by `b2 site`) and live examples, which the genesis's statement that no service worker exists contradicts.
**Migration**: See "Site: What the site does not do after increment 5": what still waits (file transfer, video chat, the release channel's publication) and what the site never does on load.
