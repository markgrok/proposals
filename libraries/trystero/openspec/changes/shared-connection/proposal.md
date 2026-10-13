## Why

Trystero shares one WebRTC connection between two peers across every room of an app that both are
in, each frame naming its room; webcpp::trystero 0.1.0 does not. There, each `room` is a world of
its own: its own peer id (drawn per room unless named), its own negotiations and its own
connections, and a link ignores every frame that names another room's token. The page lists it as
a difference and a roadmap item, and the interop lane works around it by giving each C++ room an id
of its own. A program with several rooms therefore makes several connections to the same peer,
appears under several ids on the relays, and two rooms of one app given the same id meet a Trystero
JS peer in one of them only. The owner decided on 2026-10-11 that trystero shares one connection
between the rooms of an app, as Trystero does, and that the difference goes away.

## What Changes

- **A new public `app`** (`<webcpp/trystero/app.hpp>`): one peer id per app, one connection per
  remote peer, shared by every room of the app that both peers are in. `app_config` holds what the
  rooms share (app id, self id, relays, ICE servers, `idle_timeout`, log, memory) and
  `room_options` what a room adds (password, handshake timeout, receive bound, and later its own
  memory). Rooms are joined through `app::join`, which returns a `boost::system::result<room>`.
- **BREAKING:** the standalone `room(executor, factory, room_config, room_id)` and `room_config`
  are removed. The migration is mechanical and the page states it.
- **BREAKING:** the default peer id is now one per app, not one per room: the rooms of a program
  are linked on the relays and to peers by default, and rooms that must stay unlinkable are joined
  through separate `app` objects.
- **New errors** `already_joined = 14` and `too_many_rooms = 15`; the message of
  `invalid_configuration` becomes "the configuration is unusable".
- A connection layer (`detail::app_impl`, `detail::connection`, `detail::binding`) replaces
  `detail::peer_link`: presence frames advertise a room on every connection of the app, room frames
  are routed by token to the room's binding, frames for a room not bound yet are kept briefly, and
  a kick or a departure ends a binding in its room only.
- Binding rules and a rebinding schedule: a room binds a connected peer only if that peer
  advertised the room's token itself; after a kick the pair rebinds at scheduled times (11 s
  doubling to 5 min) with only the lower id refusing meanwhile; the initiator accepts an answer to
  any of its last 8 challenges. A forged announcement binds nothing.
- Wire unchanged: the oracle lane stays byte-identical.
- Test support: the memory backend gains a virtual clock, a buffering channel, a frame filter,
  deferred opens, injection and failure hooks, and its connection ids become `std::size_t`.
- The interop lane gains the sharing scenarios against Trystero JS 0.26.0 on both lanes; the page,
  the rulebook and the header comments follow.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `trystero`: ADDED requirements for the app (`App:`), the shared connection (`Shared:`), the binding rules and the rebinding schedule (`Binding:`, `Rebinding:`), what sharing changes for security (`Security:`), the app's and a room's own resources (`Memory:`), the test support and tests (`Tests:`) and the pages (`Docs:`); MODIFIED requirements for what the genesis states for a standalone room (joining, peer id, relays, ICE servers, the configuration, the handshake, kicks, peer removal, leaving, the log, the peer link, the error enumeration and messages, the resource per room, the convenience header, the guarantees, the memory backend's description, the interop scenarios and the pages); REMOVED the requirement `Room: room_config members and defaults`.

## Impact

- **Kind:** a port: it gives the port Trystero 0.26.0's model of one connection per remote peer
  per app (`"port-of"`: Trystero 0.26.0).
- **Implementation:** a local branch, unpublished: the library's branch `shared-connection`
  (25 commits from 2026-10-11, based on `a7d04b8`, an ancestor of the genesis commit `160f431`);
  `reference/patches/` holds its 25 commits as patches and `reference/uncommitted.diff` the
  branch's uncommitted work on task 6. Nothing was pushed.
- **Dependencies:** none new.
- **Status:** implementing: tasks 0 to 5c of `reference/plan.md` are done and the tasks after them
  are not (`reference/STATUS.md`); release none.
- **Other proposals:** modifies the capability the `genesis` creates; the move of trystero onto
  webcpp::asio, webcpp::webrtc and webcpp::crypto is a change of its own that comes after this one.
