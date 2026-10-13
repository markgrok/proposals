## ADDED Requirements

### Requirement: Protocol: Objects of a protocol text hold at most 18 members
`protocol::json` (`protocol/json.hpp`) SHALL define `max_object_members` = 18, the most members a `boost::json::object` holds without its hash index, and a `static_assert` MUST tie it to `boost::json::detail::small_object_size_` so that a Boost.JSON that moves the threshold fails the build. Every object of a text parsed with `protocol::json::parse` MUST hold at most `max_object_members` members, each object counted on its own: the parse stops as it meets the 19th member of an object, before it reads that member's key, with `boost::json::error::object_too_large`, so that no object is built and what follows in the text is never read; arrays are not bounded in their elements. The parser is a `boost::json::basic_parser` over a handler of the library's own (`detail::bounded_handler`, a `value_stack` that builds the value in the caller's storage, with `max_object_size` = 18), which allocates and reports errors as `boost::json::parser` does. The largest object the protocol reads has 7 members (a Nostr event; a signal).

#### Scenario: The limit and one member more
- **WHEN** `json_bounds` parses an object of 18 members `k0` to `k17`, then one of 19
- **THEN** the first parses with 18 members, in the caller's storage, and the second sets `ec` to `boost::json::error::object_too_large` and returns a null value

#### Scenario: The parse stops at the 19th member
- **WHEN** an object of 19 members is given a further member `,"k19":{,,,` before its closing brace
- **THEN** the error is still `boost::json::error::object_too_large`, not a syntax error, because the text after the 19th member is never read

#### Scenario: Counted per object, arrays unbounded
- **WHEN** an object of 18 objects of 18 members each (342 members), an object of 18 objects of which one has 19 members, an object nested three deep whose innermost has 19 members, and an array of 100 objects of 18 members are parsed
- **THEN** the first and the array parse (the array with 100 elements), and the second and the third give `object_too_large`

### Requirement: Protocol: parse_payload reads an application's JSON without the member bound
`protocol::json::parse_payload(text, storage, ec)` SHALL parse any JSON text into a value of `storage` as `boost::json::parser` does (the behaviour `protocol::json::parse` had in 0.1.0), applying neither the length bound nor `max_object_members`, and setting `ec` and returning a null value of `storage` when the text is not one JSON value. `payload_traits<boost::json::value>` of `room.hpp`, which decodes the payload of a JSON action, MUST use it, since an application's objects may have any number of members.

#### Scenario: An object of 100 members is a payload
- **WHEN** `parse_payload` reads an object of 100 members `k0` to `k99`
- **THEN** `ec` is clear and the object has 100 members

### Requirement: Protocol: The longest signal a room parses
`signal.hpp` SHALL define `max_parsed_signal_bytes` = `max_sealed_description_bytes + 1024` = 66,560 bytes (65 KiB), the longest text of a signal a room passes to `protocol::json::parse`, and a `static_assert` MUST hold that `max_sealed_description_bytes + max_peer_id_bytes + max_offer_id_bytes + 64 <= max_parsed_signal_bytes`, so that no signal the bounds of its fields admit (a sealed offer or answer of 64 KiB with the longest ids, the 64 bytes holding the keys and the punctuation of an answer from a passive room) is refused. A longer signal is refused before it is read, with `boost::system::errc::message_size`. The longest signal measured, an answer between Trystero JS in Chrome and both backends, is 762 bytes.

#### Scenario: The signal at the bound and one byte over
- **WHEN** `json_bounds` parses `{"peerId":"p","offer":"aaa...a"}` of exactly `max_parsed_signal_bytes` with that bound, then one of one byte more
- **THEN** the first parses, and the second sets `ec` to `boost::system::errc::message_size`

### Requirement: Protocol: The longest relay text a room parses
`nostr.hpp` SHALL define `max_parsed_relay_text_bytes` = `max_parsed_signal_bytes + 2048` = 68,608 bytes (67 KiB), the longest text of a relay a room passes to `protocol::json::parse`: an event that carries the longest signal a room parses, with 2 KiB for the event's other fields, the subscription id and the escaping of the signal's quotes, which take some 600 bytes. A `static_assert` MUST hold that it is below `max_relay_message_bytes`, so that a text longer than it and not longer than 1 MiB is dropped and the connection goes on, and one longer than 1 MiB still closes the connection. The longest text measured, an event that carries an answer, is 1,244 bytes.

#### Scenario: The relay text at the bound and one byte over
- **WHEN** `json_bounds` parses `["NOTICE","nnn...n"]` of exactly `max_parsed_relay_text_bytes` with that bound, then the same text and one space
- **THEN** the first parses and the second sets `ec` to `boost::system::errc::message_size`

### Requirement: Protocol: The longest handshake message a room parses
`signal.hpp` SHALL define `max_handshake_bytes` = 1024, the longest message of the password handshake a room passes to `protocol::json::parse`, ten times the longest Trystero writes (99 bytes, `{"__trystero_pw":"response","h":"<64 hexadecimal digits>"}`). The room parses the `@_hsdata` message of a pending peer with it, and a message longer than the bound fails the handshake as a message of the wrong shape does (`error::wrong_password` for the peer, as in "Room: A peer with another password never joins").

#### Scenario: A handshake message over the bound
- **WHEN** a pending peer sends an `@_hsdata` message of 1025 bytes
- **THEN** the room treats it as a message of the wrong shape: it reports the peer with `error::wrong_password`, sends `@_leave` and drops it (`fail_handshake`)

### Requirement: Native: A remote ICE candidate is parsed under the sealed candidate bound
`native::peer_connection::add_ice_candidate` SHALL parse the candidate text it receives with `protocol::json::parse` bounded at `max_sealed_candidate_bytes` (16 KiB), since an opened candidate is shorter than its sealed form, which a signal bounds. A text over the bound, malformed, or an object of it with more than 18 members is treated as one that has no candidate: the call completes with `error::backend_failure` (the code it already used for a text with no candidate) and the negotiation goes on with the others.

#### Scenario: A candidate text over the bound
- **WHEN** `add_ice_candidate` receives a text longer than 16 KiB
- **THEN** its handler is called on the executor with `error::backend_failure`, and no candidate is added

### Requirement: Signaling: A relay text or a signal over its bound is dropped and logged
A room SHALL pass every text a relay sends to `protocol::json::parse` bounded at `max_parsed_relay_text_bytes`, and every signal (the content of an event) bounded at `max_parsed_signal_bytes`; when the parse sets `ec` the room MUST drop the text and log it, and MUST NOT close the relay's connection, make a connection or act on the message. The nostr strategy logs `nostr: dropped a text from <relay url>: <the message of ec>`, and the signaling logs `signal: dropped a signal: <the message of ec>`; a text that parses but is not a relay message or a signal is ignored without a log, as before. The reasons are `boost::json::error::object_too_large` for an object of more than 18 members and `boost::system::errc::message_size` for a text over the bound. A relay text that exceeds `max_relay_message_bytes` is still not copied and closes the connection ("Protocol: Bounds on a relay's texts").

#### Scenario: Hostile relay texts and the room goes on
- **WHEN** `hostile_relay_texts_are_refused_and_the_room_goes_on` has a relay deliver to a room that offers (id `00000000000000000000`) an event with 256 further members of keys of 128 bytes that share one digest of Boost.JSON, and an announcement whose signal has the same members besides its peer id, then a relay text and a signal one byte longer than they are parsed, and then starts a second room `BBBBBBBBBBBBBBBBBBBB` on the same relay
- **THEN** the log holds exactly once `nostr: dropped a text from <relay>: <object_too_large message>` and once `signal: dropped a signal: <object_too_large message>`, and exactly once each the same two lines with the `message_size` message; the memory resource handed out less than 8,192 bytes (a quarter of the keys' 32,768) while the room read the event of colliding members; the backend made no connection; the second room joins through the same relay (`on_peer_join` once); and no line begins `nostr: lost `

### Requirement: Actions: A JSON payload is not bounded in its objects
The decoding of a `boost::json::value` action payload SHALL accept an object of any number of members, because it uses `protocol::json::parse_payload`, and a payload of another format is unaffected. The payload is the application's, delivered after the handshake to a peer that joined.

#### Scenario: A JSON payload of 100 members
- **WHEN** a peer sends the JSON action payload `{"k0":0,...,"k99":0}`
- **THEN** the receiving room delivers a `boost::json::value` object with 100 members

### Requirement: Library: Guarantee 9, no text of a relay costs a room more than its length to read
The library SHALL hold a ninth guarantee, anchor `trystero-invariant-9`: a relay's text longer than 67 KiB, a signal longer than 65 KiB and an object of either with more than 18 members MUST be refused before they are read, logged and dropped, and the connection MUST go on, so that no object a room parses for the protocol is large enough for Boost.JSON to hash its keys, and keys crafted to share one hash cost nothing more than other keys; held by `protocol` (`json_bounds`) and by `room` (`hostile_relay_texts_are_refused_and_the_room_goes_on`: 256 keys of one hash in an event and in a signal, then a peer joins through the same relay), natively and on emscripten. The number is the diff's; "Library: The guarantees keep their anchors" and the design (D6) state how it is settled with the guarantees of `shared-connection`.

#### Scenario: The guarantee names its tests
- **WHEN** `doc/guarantees.adoc` is read
- **THEN** it has the anchor `trystero-invariant-9` on the sentence "No text of a relay costs a room more than its length to read", citing `protocol` (`json_bounds`) and `room` (`hostile_relay_texts_are_refused_and_the_room_goes_on`) and linking the anchor `trystero-protocol-bounds` of the protocol chapter

### Requirement: Tests: The protocol test checks the parse bounds
`test/protocol_test.cpp` SHALL run a case `json_bounds` after `signal_bounds`, checking: `max_object_members` is 18; an object of 18 members parses in the caller's storage and one of 19 gives `boost::json::error::object_too_large` with a null value; a text malformed after the 19th member still gives that error; the bound is per object (an object of 18 objects of 18 members, an inner object of 19, three levels deep) and arrays of 100 objects of 18 members parse; the size bound (`text.size()` accepted, `text.size() - 1` gives `boost::system::errc::message_size`, `text + "x"` with `text.size() + 1` gives `boost::json::error::extra_data`, `{` gives `boost::json::error::incomplete`); the longest signal and the longest relay text parse and one byte more gives `message_size`; and `parse_payload` accepts an object of 100 members.

#### Scenario: The case runs with the others
- **WHEN** the `protocol` test runs with the fixture path
- **THEN** `json_bounds` runs after `signal_bounds` and before `text`, and the program reports no error

### Requirement: Tests: The room test reads hostile relay texts
`test/room_test.cpp` SHALL run a case `hostile_relay_texts_are_refused_and_the_room_goes_on` last, with a `counting_resource` that also counts `allocated`, every byte it handed out. A function `colliding_keys<Word>(count, length)` generates `count` keys of `length` bytes that share one digest of Boost.JSON's object whatever its seed: Boost.JSON's MurmurHash folds each word w of a key into its state with an xor and a multiplication by an odd number after mixing it by an invertible f, so that two-word units whose words differ by f(w') = f(w) ^ top bit leave the state alike, and n units make 2^n keys of one digest; the words are printable ASCII so that a key is a JSON string, 8-byte words for 64-bit Boost.JSON and 4-byte words for 32-bit (wasm32). The case checks, for the seeds 0x1000, 0x7ffe1234 and the all-ones word, that the 256 keys of 128 bytes have one `boost::json::detail::digest`, and that the room's behaviour is as "Signaling: A relay text or a signal over its bound is dropped and logged" states. A static assertion holds that `BOOST_JSON_ARCH` equals the bits of a `std::size_t`.

#### Scenario: The case is registered last
- **WHEN** `room_test`'s `main` is read
- **THEN** it calls `hostile_relay_texts_are_refused_and_the_room_goes_on` after `a_rate_limited_relay_does_not_stop_discovery`

### Requirement: Tests: The memory relay can inject any text
`memory_relay` of `test/support/memory_backend.hpp` SHALL offer `inject(topic, text_for)`, which sends each subscription to `topic` the text `text_for` makes of its subscription id, as it is: a relay that forwards anything or makes texts of its own. The relay's own parse of what a client sends (`memory_relay::receive`) MUST be bounded at `max_relay_message_bytes`, as `protocol::json::parse` now requires a bound of every caller.

#### Scenario: A test delivers a notice no relay would
- **WHEN** a test calls `relay(url).inject(topic, [](std::string_view) { return std::string(R"(["NOTICE","x"])"); })`
- **THEN** every open subscription to `topic` receives that text unchanged

### Requirement: Docs: The bounds on parsed texts are on the page and in the rulebook
`doc/protocol.adoc` SHALL state, in the section on what a third party can make a room hold (`trystero-protocol-bounds`), that what a room parses for the protocol is bounded before it is read: a relay's text at 67 KiB (`protocol::nostr::max_parsed_relay_text_bytes`), a signal at 65 KiB (`protocol::max_parsed_signal_bytes`, the longest the bounds of its fields admit, where the longest measured are 1,244 and 762 bytes), a handshake message at 1 KiB, and an object in any of them at 18 members (`protocol::json::max_object_members`), where a `boost::json::object` starts to index its keys by a hash that holds no secret, so that keys crafted to share one hash are refused at the 19th member, before the object is built; the largest object of the protocol has 7 members; each text refused is logged and dropped and the connection goes on; an application's JSON payload is not bounded so. `doc/guarantees.adoc` SHALL hold guarantee 9, `doc/testing.adoc` SHALL describe the `protocol` row with "the bounds of a text a room parses, its length and its objects' members" and the `room` row with "a relay that sends texts too long or objects of keys that share one hash", and `AGENTS.md` SHALL state the same bounds with the measure that motivates them (a parse of keys that collide is quadratic in the text: 4 MB took 9.2 s against 4 ms) and that the `static_assert` ties the bound to Boost.JSON's `detail::small_object_size_`.

#### Scenario: The page cites the bounds
- **WHEN** `doc/protocol.adoc` is read
- **THEN** its bounds section names `protocol::nostr::max_parsed_relay_text_bytes`, `protocol::max_parsed_signal_bytes` and `protocol::json::max_object_members` and says an application's JSON payload is not bounded so

## MODIFIED Requirements

### Requirement: Protocol: JSON parsing in the caller's storage
`protocol::json::parse(text, max_bytes, storage, ec)` SHALL parse one JSON value of a protocol text into `storage`, so that the value and the parser's temporaries are allocated in that storage and not in the default resource as `boost::json::parse` would for text beyond its stack buffer. It MUST refuse a text longer than `max_bytes`, before reading it, by setting `ec` to `boost::system::errc::message_size`, and a text with an object of more than `protocol::json::max_object_members` members, as the parser meets the first member too many, by setting `ec` to `boost::json::error::object_too_large`; it sets `ec` to `boost::json::error::extra_data` when a value is followed by more text; in every failure it returns a null value of `storage`. There is no default for `max_bytes`: each caller states the longest text it reads. An application's payload is parsed with `protocol::json::parse_payload`, which applies neither bound.

#### Scenario: Malformed text
- **WHEN** `parse("{", max_bytes, storage, ec)` is called
- **THEN** `ec` is `boost::json::error::incomplete` and the value is null

#### Scenario: A text one byte over the caller's bound
- **WHEN** `parse(text, text.size() - 1, storage, ec)` is called on the text of an object of 18 members
- **THEN** `ec` is `boost::system::errc::message_size`, and with `text.size()` as the bound it is clear

### Requirement: Library: The guarantees keep their anchors
The page's section "Guarantees" SHALL number the guarantees it lists, each keeping its number and its anchor `trystero-invariant-<n>` for good, so that a comment of the code or of a test cites it as `(doc: #trystero-invariant-3)`; the eight of 0.1.0 and guarantee 9 of "Library: Guarantee 9, no text of a relay costs a room more than its length to read" are listed. Building the page (`b2 libs/trystero/doc`) MUST fail on a citation that names no anchor of it. The test files cite guarantees this way: `allocation_test.cpp` cites `#trystero-allocation`.

#### Scenario: A citation of a missing anchor fails the page
- **WHEN** a comment cites `#trystero-invariant-99` and the page is built
- **THEN** the build of `libs/trystero/doc` fails

#### Scenario: The ninth anchor resolves
- **WHEN** a comment cites `(doc: #trystero-invariant-9)` and the page is built
- **THEN** the build finds the anchor on the guarantee about a relay's texts

### Requirement: Docs: The guarantees keep their numbers and cite their tests
`doc/guarantees.adoc` (`trystero-guarantees`) SHALL list nine numbered guarantees, each with an anchor `trystero-invariant-N` that a comment of the code or of a test cites as `(doc: #trystero-invariant-N)`, and each naming the test that holds it and fails when the promise breaks: 1 the wire is Trystero 0.26.0's (the `interop` lane); 2 every protocol value equals Trystero JS's (`protocol`, the `oracle` lane, `native_sealer`); 3 a peer with another password never joins (`room`: `the_same_password_joins_and_another_does_not`, `native_room`: `another_password_never_joins`, and the interop wrong-password scenario); 4 a payload larger than a frame travels in chunks of at most 16,370 bytes, both ways (`room`: `large_payloads_travel_in_chunks_both_ways`, 100 KB each way); 5 every allocation of a room goes through its resource (`allocation`, natively and on emscripten); 6 no unit test reaches beyond the loopback (the container lanes); 7 a room refuses a replayed or reflected offer or answer (`room`: `a_replayed_answer_is_refused`, `a_replayed_offer_is_refused`, `a_reflected_offer_is_refused_as_an_answer`, `a_reflected_offer_is_refused_as_an_offer`, `a_reflected_answer_is_refused_as_an_answer`, and the interop scenarios); 8 a relay that stops reading is dropped once the unsent text would pass 4 MiB (`native_websocket`: `a_relay_that_stops_reading_is_dropped_at_the_queue_bound`, a close before 2000 texts of 64 KiB and one only; `browser_websocket_bound`; and `room`: `a_relay_that_closes_at_once_is_tried_less_and_less_often`); 9 no text of a relay costs a room more than its length to read: a relay's text longer than 67 KiB, a signal longer than 65 KiB and an object of either with more than 18 members are refused before they are read, logged and dropped, and the connection goes on (`protocol`: `json_bounds`; `room`: `hostile_relay_texts_are_refused_and_the_room_goes_on`, natively and on emscripten). A guarantee keeps its number.

#### Scenario: A citation resolves
- **WHEN** a comment cites `(doc: #trystero-invariant-3)`
- **THEN** `b2 libs/trystero/doc` finds the anchor `trystero-invariant-3` on the page, and fails on a citation naming no anchor

### Requirement: Docs: The roadmap, the changelog and the acknowledgements
`doc/roadmap.adoc` SHALL be an appendix listing what trystero does not port or build yet: media (audio and video tracks and streams, and Trystero's `@_stream`, `@_track` and `@_signal` actions with renegotiation), Trystero's other features (request actions, decisions on incoming transfers and their progress, the application's own handshake, passive rooms, one connection shared between several rooms of an app, and the other strategies, among them MQTT and BitTorrent trackers) and the external libraries from webcpp's own build (libsecp256k1, libdatachannel and OpenSSL, found where b2's command line or `.local/` says until webcpp builds them from `third_party/`). `doc/changelog.adoc` SHALL be an appendix with the section Unreleased (`trystero-changelog-unreleased`, the region `unreleased`) holding: Added a changelog, the page's last appendix, with each version's changes; Changed `config.hpp` to include `<webcpp/config.hpp>`; Added `WEBCPP_NO_EXCEPTIONS`, which disables trystero's exceptions with every webcpp library's; Fixed a relay stalling a room with JSON objects of colliding keys. `doc/acknowledgements.adoc` SHALL state that trystero ports the wire protocol of Trystero 0.26.0, written with Trystero's source open, whose MIT notice is `LICENSE-TRYSTERO.txt`; that the BIP-340 signatures are libsecp256k1's (MIT) and the native WebRTC is libdatachannel's (Mozilla Public License 2.0).

#### Scenario: The unreleased changelog
- **WHEN** `doc/changelog.adoc` is read
- **THEN** it lists four entries under Unreleased: the changelog itself, the `config.hpp` include of `<webcpp/config.hpp>`, the `WEBCPP_NO_EXCEPTIONS` macro and the fix of a relay stalling a room with JSON objects of colliding keys
