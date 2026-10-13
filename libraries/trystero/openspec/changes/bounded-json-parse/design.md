## Context

webcpp::trystero 0.1.0 (the change `genesis`) already bounds what a peer or a relay can make a room
hold: negotiations, connections, replays, the size of a relay's text (1 MiB, which closes the
connection) and the text queued for it. What it does not bound is the *work* a parse of a text
under those bounds costs. A room parses four kinds of text for the protocol (a relay's message, a
signal, a handshake message, a native candidate) with `protocol::json::parse`, which wrapped
`boost::json::parser`, and a fifth, an application's JSON payload, with the same function.

Boost.JSON's `object` stores up to 18 members in a flat array and, above that, builds a hash index
over its keys, seeded with the object's own address. The seed holds no secret, and the work in
`room_test.cpp` shows keys that share one digest under every seed (a two-word unit and its partner,
where flipping the top bit of the mixed word flips only the top bit of the state and the next word
cancels it, so n units give 2^n keys of one digest). Each key inserted then costs a comparison with
every key before it, in one bucket. The code's comments state the measure: 4 MB took 9.2 s against
4 ms for random keys, and 256 keys of 128 bytes (a text of 35 KB) cost 32,640 comparisons. The text
is far below 1 MiB, so no existing bound refuses it, and the room acted on the message.

The owner started this on 2026-10-11 on the branch `fix/json-flood` and left the work uncommitted.
This document records the decisions the diff makes; the owner has not decided the change.

## Goals / Non-Goals

**Goals:**

- No text a relay or a peer sends costs a room more than its length to read (anchor
  `trystero-invariant-9`).
- Refuse a hostile text before it is read when its length shows it, and as the parser meets the
  first member too many otherwise, building no object.
- A refused text is logged and dropped; the relay's connection stays open and the room goes on.
- An application's JSON payload keeps its freedom: its objects may have any number of members.
- The wire does not change: the oracle lane stays byte-identical.

**Non-Goals:**

- Bounding the depth of nesting or the number of array elements (Boost.JSON's parser already
  bounds depth, and arrays are not hashed).
- Replacing Boost.JSON, or seeding its hash (the seed is Boost.JSON's).
- Bounding the application's payload: it is any JSON by design.

## Decisions

### D1. Bound the members, at Boost.JSON's own threshold

`max_object_members` = 18, a `static_assert` away from `boost::json::detail::small_object_size_`.
Below it no object is ever hashed, whatever its keys, so there is nothing to collide; the rule is
independent of the seed, of the key length and of the platform's word size. Alternatives: bound the
text only (a text of 35 KB is already enough, so no useful bound on size alone); bound the total
members of a text (the largest object of the protocol has 7 members, but a per-object bound is
what the hash needs, and arrays of objects are legitimate: a `tags` array); hash the keys with a
private seeded function (a different object type, and the cost of a parse of a hostile text still
paid). The assert makes a Boost that moves the threshold fail the build instead of silently
reopening the hole.

### D2. A parser of the library's own, stopping at the 19th member

`boost::json::basic_parser<detail::bounded_handler>` with `max_object_size` = 18: the parser checks
it as it meets each member, before it reads the member's key, so a text with one member too many
stops there with `boost::json::error::object_too_large`, and what follows (even text malformed
after that point) is never read. The handler builds the value with a `value_stack` over the same
storage, so the parse allocates in the room's resource as `parse` did. Source: the handler concept
of Boost.JSON's own `parser`, which is `basic_parser<detail::handler>`.

### D3. A size bound before any read, with the caller's own number

`parse(text, max_bytes, storage, ec)` refuses a longer text with `errc::message_size` before the
parser runs. Each caller states what it reads: a relay's text `max_parsed_relay_text_bytes`, a
signal `max_parsed_signal_bytes`, a handshake message `max_handshake_bytes`, a candidate
`max_sealed_candidate_bytes`. There is no default, so a new caller must choose one. The numbers
come from the fields' own bounds, not from the measures: the longest signal is a sealed offer or
answer of `max_sealed_description_bytes` (64 KiB) with the longest ids, and 1 KiB to spare (66,560
bytes, about sixty times the longest measured, 762 bytes); a relay's text adds 2 KiB for the event's
other fields, the subscription id and the escaping of the signal's quotes (68,608 bytes; the
longest measured is 1,244). A `static_assert` ties the signal bound to the sum of its fields'
bounds, and another keeps the relay bound under `max_relay_message_bytes`, so that a text between
the two bounds is dropped and one over 1 MiB closes the connection, as before.

### D4. Drop and log, do not close

A relay's text or a signal the parse refuses is logged (`nostr: dropped a text from <relay>:
<reason>`, `signal: dropped a signal: <reason>`) and dropped. The relay's connection stays open: the
test's comment says the room "goes on", and a text of the wrong shape was already ignored without
closing; only a text over 1 MiB closes the connection, as in 0.1.0. A handshake message over its
bound fails the handshake, as a message of the wrong shape does (`wrong_password`), with no log of
its own.

### D5. The application's payload is parsed apart

`protocol::json::parse_payload` is the previous parse: any JSON text, as Trystero's `JSON.parse`
reads it. `payload_traits<boost::json::value>` in `room.hpp` uses it. The cost of a payload's
objects is the application's: it asked for a `boost::json::value`.

### D6. The guarantee takes the next number; the numbering is open

The page gains a guarantee whose anchor the diff writes `trystero-invariant-9`. The change
`shared-connection` (accepted 2026-10-11, being built) numbers its three guarantees 9 to 11 and
lists eleven in `Docs: The guarantees keep their numbers and cite their tests`. Anchors are
permanent, so whichever change is archived second takes the next free number (12 if this one
follows) and rewrites the two requirements it modifies in common (see Open Questions).

### D7. The tests build the attack, not a sample of it

`json_bounds` (protocol test) checks the bound at 18 and 19 members, per object, with arrays of any
length, and the two size bounds to the byte. `hostile_relay_texts_are_refused_and_the_room_goes_on`
(room test) generates 256 keys of 128 bytes that share one digest of Boost.JSON for three seeds
(checks it against `boost::json::detail::digest`), sends them in an event and in a signal through a
memory relay, and checks the room allocated less than a quarter of the keys' bytes while reading
them, logged each drop once, then drops a relay text and a signal one byte over their bounds, made
no new connection, and still lets a peer join through the same relay. The generator is written for
64-bit (8-byte words) and 32-bit (4-byte words, wasm32) Boost.JSON; as written it does not
terminate (Open Question 7).

## Risks / Trade-offs

- **Interop with a relay or a peer that sends more than 18 members in an object.** A Nostr event
  has 7 members and a Trystero signal 7 at most, but a relay that adds fields to an event would be
  dropped as a whole. Not measured against the public relays (`--public` lane); see Open Questions.
- **A difference from Trystero JS.** A room drops an event whose object has 19 members; Trystero
  JS reads it. The page's "Differences from Trystero" does not yet state it.
- **A Boost.JSON internal in a `static_assert`.** `detail::small_object_size_` may move; the build
  then fails with a message that says what to do. The test names `detail::digest` too. The assertion and
  `json_bounds` were seen holding with Boost 1.90.0 and 1.92.0, and the room test compiling with
  1.92.0 (`reference/STATUS.md`).
- **A library parser instead of Boost's.** About 100 lines of handler to keep in step with
  Boost.JSON's `basic_parser` concept across versions.
- **The sizes come from fields' bounds and from a few measures.** The measures (1,244 and 762
  bytes) were taken in the interop lanes against Trystero JS in Chrome; a different browser's
  longer SDP is covered by a factor of sixty.

## Migration Plan

The change is internal to the library's text handling: a program using `room`, `app` or
`action<T>` is unchanged. A caller of `protocol::json::parse` (a namespace the page marks as the
wire's, not an API) adds a `max_bytes` argument. The changelog line is "Fixed a relay stalling a
room with JSON objects of colliding keys." An implementer rebases the diff on `160f431`
(the files it touches are unchanged since `d2563a6`), runs the gates, and commits.

## Open Questions

1. **Numbering with `shared-connection`:** which change takes guarantee 9? The diff writes 9;
   `shared-connection` writes 9 to 11. Whichever is archived second renumbers, and the two
   modified requirements (`Docs: The guarantees keep their numbers and cite their tests`, and for
   this change `Library: The guarantees keep their anchors`) are merged by hand.
2. **Public relays:** does any public Nostr relay send events with more than 18 members? The
   `interop` lane's `--public` mode would show it; it was not run.
3. **The difference from Trystero JS:** the page's "Differences from Trystero" should list the
   bound (a text of a relay over 67 KiB and an object over 18 members are dropped).
4. **Candidates:** the native bound for a candidate is `max_sealed_candidate_bytes` because an
   opened candidate is shorter than its sealed form; the browser backend (`browser/backend.hpp`'s
   `add_ice_candidate`) hands the text to the page's `JSON.parse` and is not touched by the diff;
   whether it needs a bound is not addressed.
5. **The allocation bound for a signal:** the room test asserts the room handed out less than
   8,192 bytes while it read the signal of 256 colliding members as well as the event's. With a
   stand-in key set (`reference/STATUS.md`) the event read 4,821 bytes and the signal 137,946, since
   the event that carries the signal holds its content, some 40 KB, while the signal is refused.
   The assertion needs a different bound or a different measure.
6. **Instrumentation:** the sizes the diff cites were measured with an instrumented relay
   (`test/oracle/interop/relay.mjs` writing temporary files, `std::cerr` lines in the room test),
   in an earlier copy of this work; the worktree's tree dropped it, and the change does not
   include it.
7. **The key generator does not terminate:** `colliding_keys` of the room test loops forever in
   its `pair` lambda when run (`reference/STATUS.md` has the observation and the reason I found:
   the partner of a printable word differs from it by a constant whose top byte makes the
   partner's top byte non-printable, so no pair of printable words exists, for 8-byte and for
   4-byte words). The test cannot reach its assertions as it stands; the construction needs another
   way to build keys that collide (and are valid JSON strings), or another test of the same claim.
