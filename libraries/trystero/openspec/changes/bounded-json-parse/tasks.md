# Tasks

The work is uncommitted in a local worktree (`trystero-flood`, branch `fix/json-flood`, `HEAD`
`d2563a6`); `reference/uncommitted.diff` is its `git diff HEAD`. There is no plan document: the
tasks below are read from the diff. A task is checked when the diff contains it; none of the code
was reviewed or committed by the owner, and the gates in group 5 were not run
(`reference/STATUS.md` lists what this proposal's author did run).

## 1. The bounded parser (design D1 to D3, D5)

- [x] 1.1 `protocol::json::max_object_members` = 18 with the `static_assert` on `boost::json::detail::small_object_size_`, and `detail::bounded_handler` (`protocol/json.hpp`)
- [x] 1.2 `protocol::json::parse(text, max_bytes, storage, ec)` on a `basic_parser` over the handler; the length check first (`errc::message_size`), `extra_data` for trailing text
- [x] 1.3 `protocol::json::parse_payload`, the previous parse, and `payload_traits<boost::json::value>` of `room.hpp` using it
- [x] 1.4 `max_parsed_signal_bytes` (66,560) with its `static_assert`, and `max_handshake_bytes` (1,024) (`protocol/signal.hpp`)
- [x] 1.5 `nostr::max_parsed_relay_text_bytes` (68,608) with its `static_assert` below `max_relay_message_bytes` (`protocol/nostr.hpp`)

## 2. The callers (design D4)

- [x] 2.1 The nostr strategy: `parse` bounded at the relay text bound, `nostr: dropped a text from <relay>: <reason>` (`detail/nostr_strategy.hpp`)
- [x] 2.2 The signaling: `parse` bounded at the signal bound, `signal: dropped a signal: <reason>`, a parse failure separated from a signal that is not one (`detail/signaling.hpp`)
- [x] 2.3 The room's handshake: `parse` bounded at `max_handshake_bytes` (`room.hpp`)
- [x] 2.4 The native peer connection's `add_ice_candidate`: `parse` bounded at `max_sealed_candidate_bytes` (`native/peer_connection.hpp`)
- [ ] 2.5 The browser backend's `add_ice_candidate`: decide whether it needs a bound (design Open Question 4)

## 3. The tests (design D7)

- [x] 3.1 `memory_relay::inject`, and the memory relay's own `parse` given `max_relay_message_bytes` (`test/support/memory_backend.hpp`)
- [x] 3.2 `json_bounds` in the `protocol` test (`test/protocol_test.cpp`)
- [x] 3.3 `colliding_keys<Word>` and `hostile_relay_texts_are_refused_and_the_room_goes_on` in the `room` test, `counting_resource::allocated` (`test/room_test.cpp`)
- [ ] 3.4 Make the key generator terminate, or replace it (design Open Question 7)
- [ ] 3.5 Make the allocation assertion for the signal hold, or measure it otherwise (design Open Question 5)
- [ ] 3.6 A test for the handshake bound (a message of 1,025 bytes fails the handshake) and one for a JSON payload of 100 members through a room; the diff tests neither, only `parse_payload` itself
- [ ] 3.7 The interop lane: the sizes cited (1,244 and 762 bytes) were measured with an instrumented relay (`test/oracle/interop/relay.mjs` writing temporary files) in an earlier copy of this work, not in `reference/uncommitted.diff`; decide whether a permanent check of the longest observed text belongs in the lane, and drop the instrumentation

## 4. The documentation (design D6)

- [x] 4.1 `doc/protocol.adoc`: the bounds paragraph in the section on third parties; `doc/guarantees.adoc`: guarantee 9; `doc/testing.adoc`: the two table rows; `AGENTS.md`: the bullet with the measure
- [x] 4.2 `doc/changelog.adoc`: "Fixed a relay stalling a room with JSON objects of colliding keys."
- [ ] 4.3 `doc/differences.adoc`: list the bound as a difference from Trystero JS (design Open Question 3)
- [ ] 4.4 Settle the guarantee number with `shared-connection` (design D6, Open Question 1)

## 5. The gates (none run for this proposal, beyond `reference/STATUS.md`)

- [ ] 5.1 Review of the diff by the owner; rebase on the published `160f431` (the six files it shares with `d2563a6` are unchanged since)
- [ ] 5.2 `b2` lint, the unit tests on native and emscripten, with and without exceptions (`protocol`, `room`, `allocation`, the `browser_*` tests), and the gcc-14 and clang-linux-18 container lanes
- [ ] 5.3 The `oracle` lane byte-identical; the `interop` lanes on both backends, and `--public` if the owner wants Open Question 2 answered
- [ ] 5.4 The page builds (`b2 libs/trystero/doc`) with guarantee 9 cited from the code
- [ ] 5.5 Commit with the changelog line, as `R`
