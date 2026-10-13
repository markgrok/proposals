# Tasks

The tasks of `reference/plan.md`, in its order and its numbering (22 tasks in five increments); a
checked task is done in the local branch `shared-connection`, whose 25 commits are
`reference/patches/` (`0001` to `0025` in commit order, applying on `a7d04b8`, an ancestor of the
published `160f431`). Task 6 was
started and stopped on 2026-10-11 with uncommitted work, so it is not checked
(`reference/STATUS.md`).

## 1. Baseline (plan: Task 0)

- [x] 1.1 The worktree of the superproject, the library's branch `shared-connection` from `a7d04b8`, the ledger, the baseline of every lane (plan: Task 0; no commit in the library)

## 2. Increment 1: the connection layer, one room per app (plan: Tasks 1 to 4)

- [x] 2.1 The memory backend's hooks: `std::size_t` ids, `on_made`, `peer_of`, the frame filter (deliver, drop, hold) with `release`, `defer_opens` and `open`, `inject`, `fail`, a buffering channel draining `drain_per_post` bytes per post, the virtual clock with `advance` and `max_steps`, and their tests (plan: Task 1; patches `0001`, `0002`)
- [x] 2.2 `detail::connection` and `detail::binding`: decoding of every envelope, the remote's advertised tokens, kept frames, routing by token, the drain order, the end of a connection failing every waiter, and their tests, and the explicit search of the libraries in `build.jam` (plan: Task 2; patches `0003` to `0006`)
- [x] 2.3 `app_config`, `room_options` and `detail::app_impl`: the self id, the registry, adoption that ends a stale connection first and is re-entrant without recursion (plan: Task 3; patches `0007` to `0010`)
- [x] 2.4 The room on the connection layer, `peer_link` removed, `presence(0)` and kicks scoped to the room, a connection's end naming the connection (plan: Task 4; patches `0011` to `0017`)

## 3. Increment 2: the app (plan: Tasks 5a to 12)

- [x] 3.1 The tests' shared helpers (`test/support/rooms.hpp`) and the virtual clock in them (plan: Task 5a; patches `0018` to `0020`)
- [x] 3.2 The two new errors `already_joined = 14` and `too_many_rooms = 15` (plan: Task 5b; patch `0021`)
- [x] 3.3 `app`, `app::join`, the standalone room and `room_config` removed, every test, example and interop peer moved to `app`, the setup reports delivered once to the current handler (plan: Task 5c; patches `0022` to `0025`)
- [ ] 3.4 Binding by presence and by kept frames, safely (plan: Task 6; started, uncommitted)
- [ ] 3.5 Signaling reuse, the binding conditions and a bound room's negotiation (plan: Task 7)
- [ ] 3.6 Departures, kicks and a connection's end, one room at a time (plan: Task 8a)
- [ ] 3.7 The farewell, joining during it and the app's users (plan: Task 8b)
- [ ] 3.8 Rebinding times, the lower id's backoff and the higher id's dropping window (plan: Task 9)
- [ ] 3.9 The handshake across a rebinding, and a kept `@_leave` (plan: Task 10)
- [ ] 3.10 Crossed connections, flood isolation and the drain order (plan: Task 11)
- [ ] 3.11 A passwordless room is open to the app's other rooms; the increment's gates (plan: Task 12)

## 4. Increment 3: allocation (plan: Task 13)

- [ ] 4.1 `room_options::memory`, a binary payload copied once into a room's own resource, the counting test and the room whose resource returns to zero (plan: Task 13)

## 5. Increment 4: interop (plan: Tasks 14a to 14c)

- [ ] 5.1 The interop peers' and the page's parameters, events and hooks; scenario 1, two rooms on one connection (plan: Task 14a)
- [ ] 5.2 Sharing scenarios 2, 3, 5 and 6 (plan: Task 14b)
- [ ] 5.3 Security and timing scenarios 4, 7, 8 and 9 (plan: Task 14c)

## 6. Increment 5: the pages (plan: Tasks 15 and 16)

- [ ] 6.1 The page, the rulebook and the header comments: differences, roadmap, usage, protocol, design, allocation, guarantees, testing, overview, quick start, browser, native, README, AGENTS.md (plan: Task 15)
- [ ] 6.2 Every lane from scratch, a whole-branch review against the specification, and the gates G1 and G2 of the controller (plan: Task 16)

## 7. Beyond the plan

- [ ] 7.1 Rebase the branch onto the genesis commit `160f431` (it is based on `a7d04b8`), dropping the cherry-pick `6b1e4ba` for the owner's ruling that `4aef1c0` supersedes it
