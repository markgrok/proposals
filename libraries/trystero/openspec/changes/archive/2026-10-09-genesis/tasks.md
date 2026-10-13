# Tasks

Every task of 0.1.0, each pointing to its task in `reference/plan.md` (stage 4 of webcpp's plan,
written 2026-10-09) or to its commits in the library's repository (81 commits, `f3a653e` of
2026-10-09 to `160f431` of 2026-10-12); a checked task is done in `reference/source/`. Plan Tasks
0 to 4 and 14 were work of the superproject (machine-local dependencies, exceptions on every
target, Emscripten as a tested target of the build and CI, the lint and the reference reading
Emscripten-only and native-dependency headers, publication) and Task 13 belongs to another
library; they are listed where the library depends on them.

## 1. The superproject the library builds on (plan: Tasks 0 to 4)

- [x] 1.1 Machine-local dependencies: emsdk, libsecp256k1, libdatachannel, OpenSSL (plan: Task 0)
- [x] 1.2 Exceptions on every target, a user's `exception-handling=off` still building (plan: Task 1)
- [x] 1.3 Emscripten as a tested target of the build: `webcpp.link`, `webcpp.drive`, no launcher (plan: Task 2)
- [x] 1.4 Emscripten in CI: the `emsdk` action and the lane (plan: Task 3)
- [x] 1.5 The lint and the reference read Emscripten-only and native-dependency headers (plan: Task 4)

## 2. The library moved (plan: Tasks 5 to 7)

- [x] 2.1 Moved as `webcpp::trystero`, built by b2 without CMake: headers, `build.jam`, examples, `LICENSE-TRYSTERO.txt`, `meta/` (plan: Task 5; commits `f3a653e`, `a1ee534`)
- [x] 2.2 The tests on lightweight_test, natively and on Emscripten; the native tests on the loopback alone (plan: Task 6; commits `aff244c`, `17b7ba3`)
- [x] 2.3 The oracle and the interop lanes, pinned to Trystero 0.26.0; the driver runs any headless Chrome and removes its profile (plan: Task 7; commits `2a63141`, `1f7372a`, `480852d`)

## 3. Allocation (plan: Task 8)

- [x] 3.1 Every allocation of a room through `room_config::memory`: `json_resource`, `handler`, `detail::post` (commit `1a0eebd`)
- [x] 3.2 The resource outlives the operations a room leaves pending, and the counting proof counts what it frees (commits `f6981f4`, `e814f73`, `226c6fe`, `0867254`)

## 4. Lint, documentation and CI (plan: Tasks 9 to 12)

- [x] 4.1 Formatted, clang-tidy clean, the world it reaches marked, the native timer, sealer and WebSockets analysed without exceptions (plan: Task 9; commits `6409e03`, `bcfade6`, `68c3115`, `27ac1ab`, `41a7a5d`, `c37133f`)
- [x] 4.2 Every public symbol has a Doc Comment, and MrDocs builds the reference strict (plan: Task 10; commits `6c06ab4`, `7460928`, `9b9a14a`)
- [x] 4.3 The page on the shared documentation toolchain, its examples and the review's fixes (plan: Task 11; commits `a2a6ed0`, `fda2cda`, `737edd9`)
- [x] 4.4 AGENTS.md, README and CI through the superproject's library workflow, with the external dependencies installed by actions (plan: Task 12; commits `2d674d2`, `a9d2063`)
- [x] 4.5 Published at `webcpporg/trystero` (plan: Task 14)

## 5. Hardening after the move (commits, no plan task)

- [x] 5.1 The browser backend: a malformed ICE candidate, a refused relay URL, a sealer that never completes inside the call, a data channel message that is no frame, no JavaScript exception entering WebAssembly, the stack recovered after a page function's exception (commits `a59a6fd`, `76a56d3`, `6ea909f`, `5163f99`, `85429d2`, `34b6cc4`, `bbafd75`)
- [x] 5.2 ICE servers: one WebRTC refuses is dropped at join and reported, schemes compared without regard to case (commits `7b53dd5`, `0d48c2e`)
- [x] 5.3 A room moved over a joined one leaves it (commit `4bee49c`)
- [x] 5.4 Bounded signaling: announcements from fresh ids, fields of a signal, nothing changes a negotiation before it opens with the key, unopened signals reported paced, connections bounded per room and per page, offers asked at most once per 5.3 s per relay (commits `65726c4`, `3c91a0e`, `d1c5f0d`, `732a388`, `9df7ec1`, `9aad713`, `a95e41d`)
- [x] 5.5 Replays and reflections: a room refuses a sealed offer or answer it already acted on, and its own sent back to it (commits `dd440f9`, `7bc78ef`, `915ed1d`, `a7d04b8`)
- [x] 5.6 Relays: a text longer than 1 MiB closes the connection; a relay that stops reading or answering is dropped, natively and in the browser; the reconnection delay comes back down only after a connection lasted (commits `0da6b79`, `27560b1`, `09c424d`, `071ffaa`, `10037fb`)
- [x] 5.7 Native backend: a timer that is cancelled never fires; a WebSocket whose connection ends during a write keeps its queue; the interop driver names a peer killed by a signal (commits `15e8887`, `0e4525e`, `cde424e`)
- [x] 5.8 Flood tests that hold on a busy machine and on every standard library (commits `4918ac3`, `1d8cb7d`, `358c367`, `d2563a6`, `571a2e4`)
- [x] 5.9 Interop: two rooms of one app each with a peer id of its own (commit `2a903ce`)

## 6. Toolchains and release (commits, no plan task)

- [x] 6.1 Dependencies from b2's command line or `.local/`, left out when missing, stopping the build under `webcpp-require-external` (commits `b2ff888`, `9b54857`, `c981557`)
- [x] 6.2 The browser backend's tests without exceptions, as every library supports both builds (commit `7eead1f`)
- [x] 6.3 MSVC 14.3 and 14.5 build the library, its tests and its examples; Emscripten's reference read (commits `e5685a3`, `0457c42`, `4deedc5`, `79e2796`)
- [x] 6.4 A changelog, `config.hpp` including `<webcpp/config.hpp>`, `WEBCPP_NO_EXCEPTIONS` disabling trystero's exceptions, the maintainer's public address (commits `f3d0509`, `454a665`, `160f431`)
