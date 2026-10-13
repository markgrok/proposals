# webcpp::trystero: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal took trystero in as a retroactive genesis. `plan.md` is the
stage 4 plan of 2026-10-09 that moved the library into webcpp (its text is revised and names tasks
of the superproject too); `../tasks.md` is the checklist; `source/` is the library's tree at
`160f431`, exactly `git archive 160f431` of the library's repository.

## Where it stands

| What | Where |
| --- | --- |
| The library | `github.com/webcpporg/trystero`, commit `160f431` of 2026-10-12 ("meta: the maintainer's public address"), release 0.1.0 |
| History | 81 commits, all as `R`, `f3a653e` of 2026-10-09 ("moved as webcpp::trystero, built by webcpp's b2 without CMake") to `160f431` |
| The superproject | holds the library as the submodule `libs/trystero` |
| The page | built from `doc/trystero.adoc` and its sections by the superproject's documentation toolchain, with a MrDocs reference; its changelog (`doc/changelog.adoc`) lists three "Unreleased" lines |

The proposal's metadata (status `implemented`, release `0.1.0`) is as the controller gave it; this
proposal did not look at the remote repository, a CI run or a release.

### What was measured

Nothing was run for this proposal: the requirements were written by reading `source/`, its tests,
its documentation and its history. What the sources themselves state as measured:

- `AGENTS.md` and the page: Chrome counts a peer connection against its 500 until a collection
  frees it (headless Chrome: the 501st throws "Cannot create so many PeerConnections"), hence the
  browser backend's census of 256; the interop lanes passed under `docker run --network none` with
  one interface holding the default route; a full Chrome for Testing 156 contacts Google unless the
  driver resolves no host but 127.0.0.1, while the headless shell asks nothing.
- The page, guarantee 6: the container lanes (`gcc-14`, `clang-linux-18`) pass every unit test in a
  container with no network but the loopback.
- The interop lane has sixteen scenarios per C++ peer (plus three for the native peer), as
  `AGENTS.md` and `doc/testing.adoc` count them.

## Open items of 0.1.0

1. **Features not ported** (`doc/differences.adoc`, `doc/roadmap.adoc`): audio and video with
   renegotiation; request actions (`kind: 'request'`); `onReceive`, the progress callbacks and
   cancelling a send; `onPeerHandshake`; passive rooms; one connection shared by the rooms of an
   app (the change `shared-connection`); every strategy but Nostr; `getRelaySockets` and the relay
   reconnection controls; `trickleIce: false`, `turnConfig` and `rtcPolyfill`.
2. **The external libraries** (libsecp256k1, libdatachannel, OpenSSL) are found where b2's command
   line or the superproject's `.local/` says, until webcpp builds them from `third_party/`.
3. **What a flood still does** (`AGENTS.md`, "Bounded signaling"): a flooder who keeps sending new
   ids takes each negotiation place and each connection of the window as it frees, so new peers can
   be kept out for as long as it lasts, and a joined peer whose connection drops comes back as a new
   peer and is kept out the same way. A watcher without the key can keep two peers apart from their
   first meeting. Only a wire that seals the offer id and both peer ids closes this, which
   Trystero would have to share (reported upstream).
4. **libdatachannel's standard library**: it must be built with the same one as the program that
   links it, so the native backend's tests do not link with libstdc++ against a libc++ build.

## How to resume

1. Work in a worktree of the superproject (`git worktree add ../webcpp-trystero-<slug> -b
   lib/trystero-<slug> main`, then `git submodule update --init`), following its `AGENTS.md`,
   chapter "Working on a library". `libs/trystero` is the published repository; `source/` is the
   same tree for reading without a checkout.
2. Find the three external libraries as `source/README.md` and `doc/building.adoc` say, then build
   from scratch (`b2 -a libs/trystero/test libs/trystero/example`, and with
   `toolset=emscripten`); the oracle and interop lanes need Node and a headless Chrome.
3. The wire never changes (`source/AGENTS.md`, "The original, and its wire"): a change that touches
   `protocol/` must keep the `oracle` lane byte-identical, and `update-expected` is the only writer
   of `expected.json`.
4. Before saying a task is done: the superproject's lint, both targets' tests, the oracle and both
   interop lanes, the page.
5. The next change in line is `shared-connection`, which modifies the capability this genesis
   creates.
