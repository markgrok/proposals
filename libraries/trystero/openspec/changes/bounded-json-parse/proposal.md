## Why

A relay is a third party, and so is any peer: whatever it sends, a room parses before it knows
what it is. webcpp::trystero 0.1.0 bounds the *size* of a relay's text (1 MiB, which closes the
connection) but not what a parse of a smaller text costs. Boost.JSON's `object` indexes its keys
by a hash once it holds more than 18 members, a hash seeded with the object's own address, which
holds no secret; sets of keys exist that collide under every seed, and a text of them makes the
parse quadratic in its members (the code's comments state 4 MB parsed in 9.2 s against 4 ms for
random keys of the same size, and 256 colliding keys of 128 bytes, a text of 35 KB, costing 32,640
comparisons). A hostile relay, or a peer whose signal a relay forwards, can therefore stall a room
that is working, with a text well under the 1 MiB bound. Trystero JS reads the same texts with
`JSON.parse` and bounds nothing of them; the hazard is the port's parser, not the wire. The work was started on 2026-10-11 on the branch `fix/json-flood`, after
the interop flood work ending at `d2563a6`, and left uncommitted.

## What Changes

- **A bound on an object's members:** every object of a text the room parses for the protocol
  holds at most `protocol::json::max_object_members` = 18 members (the most a `boost::json::object`
  holds without its hash index, tied to Boost.JSON's `detail::small_object_size_` by a
  `static_assert`). The parser is a `boost::json::basic_parser` with a handler of trystero's whose
  `max_object_size` is 18: it stops at the 19th member, before reading its key, with
  `boost::json::error::object_too_large`, and no object is built. The largest object of the
  protocol has 7 members.
- **A bound on the text parsed, before it is read:** `protocol::json::parse(text, max_bytes,
  storage, ec)` refuses a longer text with `boost::system::errc::message_size`. The bounds are
  `protocol::nostr::max_parsed_relay_text_bytes` (68,608 = 67 KiB, a relay's text),
  `protocol::max_parsed_signal_bytes` (66,560 = 65 KiB, a signal),
  `protocol::max_handshake_bytes` (1,024) and, for a candidate the native backend opens,
  `max_sealed_candidate_bytes` (16 KiB). The measured longest relay text is 1,244 bytes and the
  longest signal 762 bytes.
- **BREAKING (for callers of `protocol::json::parse`, an internal namespace):** `parse` takes
  `max_bytes` as its second argument. `protocol::json::parse_payload(text, storage, ec)` is the
  unbounded parse of an application's JSON payload (`boost::json::value` actions), whose objects
  may have any number of members.
- **Dropped and logged:** a relay text or a signal the parse refuses is logged (`nostr: dropped a
  text from <relay>: <reason>`, `signal: dropped a signal: <reason>`) and dropped, and the relay's
  connection goes on; a handshake message over its bound fails the handshake as one of the wrong
  shape does. A text over 1 MiB still closes the connection, as before.
- **A ninth guarantee** on the page, anchor `trystero-invariant-9`: no text of a relay costs a room
  more than its length to read.
- Test support: the memory relay can `inject` any text; a test generates keys that share one
  Boost.JSON digest under any seed (64-bit and 32-bit words).
- Tests: `json_bounds` in the `protocol` test and `hostile_relay_texts_are_refused_and_the_room_goes_on`
  in the `room` test.
- The page (`protocol`, `guarantees` and `testing` sections), the rulebook (`AGENTS.md`) and the
  changelog follow.
- Wire unchanged: the oracle lane stays byte-identical.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `trystero`: ADDED requirements for the member bound, `parse_payload`, the three text bounds, the
  native candidate bound, the dropped-and-logged behaviour, the handshake bound, the ninth
  guarantee, the memory relay's `inject`, the two tests and the pages; MODIFIED requirements for
  `parse` (`Protocol: JSON parsing in the caller's storage`), the guarantees (`Library: The
  guarantees keep their anchors`, `Docs: The guarantees keep their numbers and cite their tests`)
  and the changelog (`Docs: The roadmap, the changelog and the acknowledgements`).

## Impact

- **Kind:** a port: a hardening of a library that ports Trystero 0.26.0 (`"port-of"`: Trystero
  0.26.0); it changes no byte of the wire.
- **Implementation:** uncommitted work in a local worktree, unpublished: `trystero-flood`
  (branch `fix/json-flood`, `HEAD` `d2563a6` of 2026-10-11, an ancestor of the published `160f431`),
  15 files, 596 insertions and 14 deletions; `reference/uncommitted.diff` is its `git diff HEAD`.
  Nothing was committed or pushed.
- **Dependencies:** none new; it names one internal constant of Boost.JSON
  (`boost::json::detail::small_object_size_`) in a `static_assert`, and its test names
  `boost::json::detail::digest`.
- **Status:** draft (the owner has not decided it); release none.
- **Other proposals:** modifies the capability the `genesis` creates. `shared-connection` also
  numbers guarantees 9 to 11 and modifies `Docs: The guarantees keep their numbers and cite their
  tests`: whichever of the two is archived second renumbers (`design.md`, D6).
