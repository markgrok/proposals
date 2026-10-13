# webcpp::trystero, bounded JSON parse: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal took the owner's uncommitted work in. There is no plan
document: `../design.md` records the decisions the diff makes, `../tasks.md` is read from the diff.
`uncommitted.diff` is the work.
A change carries only its diff from the commit it was requested against, never the library's whole
tree: the tree is `d2563a6` with `uncommitted.diff` applied.

## Where it stands

The work is **uncommitted in a local worktree, unpublished**.

| What | Where |
| --- | --- |
| The work | the library's worktree `trystero-flood` (a sibling of the superproject checkout; branch `fix/json-flood`, `HEAD` `d2563a6` of 2026-10-11 17:32, "the interop flood scenario holds on a busy machine") |
| Its size | 15 modified files, 596 insertions and 14 deletions: `include/webcpp/trystero/{detail/nostr_strategy,detail/signaling,native/peer_connection,room}.hpp`, `protocol/{json,nostr,signal}.hpp`, `test/{protocol_test,room_test}.cpp`, `test/support/memory_backend.hpp`, `doc/{changelog,guarantees,protocol,testing}.adoc` and `AGENTS.md`; files last written 2026-10-11 18:27 |
| The base | `d2563a6` is an ancestor of the published `160f431`; the published library has not touched `nostr.hpp`, `signal.hpp`, `protocol_test.cpp`, `room_test.cpp` or `memory_backend.hpp` since, so their hunks apply on `160f431` as they are; the other files were not compared with `160f431` |
| Pushed or committed | no; no CI ran |

`uncommitted.diff` is `git diff HEAD` of that worktree, taken on 2026-10-13.

## What was measured

By the owner's comments in the diff (not re-run here): a parse of keys that share one digest of
Boost.JSON's object took 9.2 s for 4 MB against 4 ms for random keys of the same size; 256 such
keys of 128 bytes, a text of 35 KB, cost 32,640 comparisons; 32,768 keys of 240 bytes share one
digest. Not stated anywhere: which machine, Boost version or build.

### What this proposal's author ran (2026-10-13, macOS arm64, Apple clang, `-std=c++20`, outside b2)

Nothing was run through the library's gates (no b2, no lint, no emscripten, no interop or oracle
lane, no sanitizer, no container lane). Scratch builds of the worktree's headers and tests, with
Homebrew's Boost and the superproject's native libsecp256k1, linked with `boost/json/src.hpp`:

- `json_bounds` of `protocol_test.cpp`, extracted with its helper into a scratch program: **passes**
  ("No errors detected"), with Boost 1.92.0 and with 1.90.0; `json.hpp`'s `static_assert` on
  `small_object_size_` holds on both, and the constants print as `max_parsed_signal_bytes` 66,560
  and `max_parsed_relay_text_bytes` 68,608.
- `room_test.cpp` as it stands (Boost 1.92.0, `-O1`): **does not finish**. It spins at 100 percent
  of a core in `colliding_keys<unsigned long>`, in the lambda that draws a pair of words (observed
  16 minutes for the whole program and 5 minutes for the hostile case run alone, which I stopped).
  The cause I found: the partner of a word, `unmix(mix(x) ^ top)`, always equals `x + D` or `x - D`
  with `D` = `0x71581a64e6430000`; adding that constant moves the top byte of a printable word
  above 0x7e, so `printable(x) && printable(partner)` held for none of 5,000,000 random words, for
  the 64-bit mix and for the 32-bit one (`0x5bd1e995`, 24) alike. I did not find a construction
  that works; the idea (colliding keys exist) is the owner's and I did not test it.
- `room_test.cpp` with the call of `hostile_relay_texts_are_refused_and_the_room_goes_on` removed:
  every other case **passes** (19 s, "No errors detected"), which exercises the changed callers
  (`nostr_strategy`, `signaling`, the handshake) on the existing cases.
- The hostile case with a stand-in key set (256 distinct keys of 128 bytes, no collisions, and the
  digest check removed): every assertion holds (the two drops are logged once each with the
  `object_too_large` message, the two over-length drops with the `message_size` message, no
  connection is made, a peer joins, nothing is lost) **except** `in_signal < 8192`: the room handed
  out 4,821 bytes while reading the event and 137,946 while reading the signal (the members' text is
  34,048 bytes; the event that carries the signal holds it as a string of some 40 KB). This says
  nothing about colliding keys, whose cost the stand-in does not reproduce; it says the allocation
  assertion for the signal looks mis-set.

## Open items

1. The key generator does not terminate (design Open Question 7): the hostile case cannot run.
2. The signal's allocation assertion (Open Question 5).
3. Numbering of guarantee 9 against `shared-connection` (D6, Open Question 1).
4. No test covers the handshake bound or a JSON payload through a room (tasks 3.6); the browser
   backend's `add_ice_candidate` is unbounded (task 2.5); the page's list of differences does not
   name the bound (task 4.3).
5. The public relays were not looked at (Open Question 2).
6. The emscripten targets were not built or run; the generator is written for both word widths and
   was never run for either.

## How to resume

1. Work in a worktree of the superproject (`git worktree add ../webcpp-trystero-<slug> -b
   lib/trystero-<slug> main`, then `git submodule update --init`), following its `AGENTS.md`,
   chapter "Working on a library". Create the library's branch on the published `160f431`, apply
   `uncommitted.diff` (`git apply`; it is `git diff HEAD` of a tree at `d2563a6`, whose files
   it touches are those named above), and read it whole: the owner has not reviewed it.
2. Fix the key generator first (task 3.4): the hostile case is the guarantee's test. Check any
   construction against `boost::json::detail::digest` for several seeds, as the case already does,
   on both word widths, and keep the keys valid JSON strings.
3. Then the allocation assertion (3.5), the missing tests (3.6) and the numbering (4.4).
4. The wire never changes (`source/AGENTS.md` of the genesis, "The original, and its wire"): the
   `oracle` lane stays byte-identical; the change touches no fixture.
5. Before saying a task is done: the superproject's lint, both targets' tests with and without
   exceptions, the oracle and both interop lanes, the page; then commit as `R` with the changelog
   line the diff already holds (`doc/changelog.adoc`), without adding an agent as co-author.
