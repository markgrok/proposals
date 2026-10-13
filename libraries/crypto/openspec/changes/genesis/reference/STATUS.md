# webcpp::crypto: where the implementation stands, and how to resume

Written 2026-10-13, when this proposal was made from the owner-approved specification and the
plan. `spec.md` is the specification (approved by the owner on 2026-10-11); `plan.md` is the
implementation plan of 43 tasks in five increments (status draft, written 2026-10-11), every task
with its files, interfaces, steps and code; `../tasks.md` is the checklist. **The reference code
is the code written in the plan's tasks:** Task 1 (`build.jam`, the test Jamfile's `crypto-run`,
`config.hpp`), Task 2 (`error.hpp`), Task 3 (`equal.hpp`, `detail/wipe.hpp`), Task 4
(`random_device.hpp`, `detail/entropy.hpp`, `detail/windows.hpp`), Task 5 (`third_party/Jamfile`,
`src/canary_bearssl.c`, `detail/canary.h`), Tasks 6 to 8 (`backends/bearssl.hpp`, `aes.hpp`,
`gcm.hpp`), Task 12 (`backends/openssl.hpp`), Task 14 (`aes256_gcm.hpp`), Task 15 (the memory
adapter), Task 21 (`webcpp.same-program`), Task 27 (`secp256k1.hpp`), Task 29
(`backends/ctaes.hpp`), Task 35 (`chacha20_poly1305.hpp`) and Task 38 (`backends/botan.hpp`).
There is no `source/` directory: nothing is implemented.

## Where it stands

- **The library's repository has no commit,** and `libs/crypto` does not exist anywhere.
  `webcpporg/crypto` has not been created (the plan's gate G1) and the submodule has not been
  added (G2). The plan's "Global Constraints" say that until then the superproject
  ignores the library through its `.git/info/exclude`.
- **No task is done.** The plan has 240 unchecked steps and none checked.
- **What exists on this machine** (observed on 2026-10-13): a superproject worktree
  `~/dev/webcpp-crypto` on branch `lib/crypto` at `a4e5bc8`, with a clean status, holding
  `third_party/Jamfile` and `tools/node-test/include/webcpp/test/exit_status.hpp` (the header
  Task 1's first test needs). It has no `libs/crypto` and no `.superpowers/sdd/WEBCPP_CRYPTO_PLAN/`
  ledger, so Task 0's first step looks made and its ledger steps are not. Whether that base
  satisfies Task 0a's gates (the owner's rewrite of `main`, the identity check) is not
  established here; run the plan's checks.
- **Specification status:** accepted by the owner on 2026-10-11 after an independent review and a
  second review of the memory option; two departures were answered by the owner the same day
  (OpenSSL's and Botan's `make` allocate outside the user's allocator; OpenSSL's shared or
  static link is a b2 feature), the other rulings are the controller's, listed in spec
  section 10.
- **Owner gates the plan waits on:** the rewrite of `main`'s history (before Task 0a); O2, the
  heavy lanes at a nightly level (before Task 25b); O3, NIST's archives pinned by URL and
  SHA-256 (before Task 9); O5, the departures C14, C16 and C17 and `brew install botan` (before
  Tasks 22, 18 and 37); G1 and G2, the repository and its submodule.

## What was measured, and when

Everything below is stated in `spec.md` or `plan.md`; none of it was re-run for this proposal.

- **2026-10-11, macOS arm64, Apple clang 21, OpenSSL 3.6.4 (spec, PROBE-1011 and PROBE-R3):**
  the first OpenSSL `make` of a process made 6,061 allocations (and 192 reallocations under the
  memory functions, 4,621 blocks live afterwards); a second `make` 2; seals and opens 0;
  OpenSSL's GCM accepts IVs of 1 to 128 bytes and refuses 129 and more; with the functions
  installed libcrypto called the C library's allocator 0 times and without them 7,584 times;
  `CRYPTO_set_mem_functions` returns 0 after any allocation by OpenSSL's own functions and 1
  after the first make through installed ones; an `ERR_set_mark` before the install makes it
  return 0. A failure in OpenSSL's first initialisation is permanent (n = 0, 5, 50).
- **2026-10-11, the plan's own measurements M1 to M7** (macOS arm64, b2 5.5.3): `/third_party`
  and `/webcpp/third_party` both resolve from one Jamroot; a skipped program does not evaluate a
  library's stop; NIST's archives' sizes and SHA-256; RFC 8439's two vectors through OpenSSL
  3.6.4; an interposer linked at launch counts the same as `DYLD_INSERT_LIBRARIES`; upstream
  commits (Wycheproof `12fd3aaf33eb5fa1f52e026912ee00c054f9d984`, `bitcoin/bips`
  `927b6de9915c9262615a6399de51b200f81e5aa4`, BearSSL's mirror master); and the tree at the time.
- **Throughput and sizes (spec section 5 and 8.8, from the owner's research):** AES-GCM on
  x86-64 (AMD EPYC 7763) 3,333 to 3,509 MiB/s on OpenSSL against 2,373 to 2,496 on BearSSL;
  Linux aarch64 2,821 to 2,825 against 92.4 to 92.5; macOS-15 (Apple M1) 5,571 to 5,908 against
  90.1 to 94.1; BearSSL 146 to 160 MiB/s on WebAssembly. libsecp256k1 verification 2,937
  against 1,744 per second on wasip2 with large against small tables; an Emscripten program of
  1,211,251 bytes with the large tables and 141,922 with the small ones.
- **Not measured yet (the spec says so):** the Windows branch of the memory option (recognising
  OpenSSL's defaults through `GetProcAddress`, `OPENSSL_cleanup` at a DLL's unload), the counts on
  OpenSSL 3.0.13 (the CI's Linux version), and MSVC's messages.

## How to resume

Prerequisites are the superproject's (its `AGENTS.md`, chapter 1): Boost 1.92, b2 5.5.3, a
C++20 compiler, wasi-sdk 34, wasmtime 47.0.3, Emscripten 6.0.11 with node 26.7.0, chrome-headless-shell
155.0.8059.39, Python 3.9 or newer, a `.local/user-config.jam`, and for the optional lanes Valgrind
and a machine OpenSSL 3 (and, for Botan, `brew install botan` once the owner agrees).

1. **Settle the base** as the plan's "The base, the order of the tracks" says: the owner's
   rewrite of `main`, then Task 0a (integrate `infra/shared` into `main`), then asio's Task 2
   (the track that provides `webcpp::test::exit_status`), then Task 0 (the worktree, the
   ledger, the baseline).
2. **Start with Tasks 1 to 4** (skeleton, errors, `equal`, `random_device`), each: the failing
   test first and seen failing, the implementation, the lanes, the lint, the commit, with the
   plan's commit rules (every commit as `R <github.overboard312@passmail.net>`, no trailer,
   staged by path).
3. **Follow the plan's order** through increment 1 (Tasks 5 to 25b), then increments 2 to 5
   (Tasks 26 to 39). Increment 3 (trystero) waits for trystero's shared-connection change to have
   integrated; Task 25a waits for asio's Task 21.
4. **Gates:** the controller alone, with the owner's agreement, creates `webcpporg/crypto`,
   adds the submodule and pushes (G1, G2, G4).

### The commands

From the superproject worktree's root. The plan writes `$B2` for
`env -u CPATH -u CPLUS_INCLUDE_PATH -u C_INCLUDE_PATH b2 --user-config=.local/user-config.jam`.

| What | Command |
| --- | --- |
| one test, while working on it | `$B2 libs/crypto/test//<name>` |
| native, from scratch | `$B2 -a --build-dir=bin/lane-native libs/crypto/test libs/crypto/example` |
| native with gcc 15 | `$B2 -a --build-dir=bin/lane-gcc toolset=gcc-15 libs/crypto/test libs/crypto/example` |
| wasip2 | `$B2 -a --build-dir=bin/lane-wasip2 toolset=clang-wasip2 testing.launcher=wasmtime libs/crypto/test libs/crypto/example` |
| wasip3 | the same with `clang-wasip3` and `bin/lane-wasip3` |
| Emscripten | `$B2 -a --build-dir=bin/lane-emscripten toolset=emscripten libs/crypto/test libs/crypto/example` |
| an own lane on a target | the target's command with `libs/crypto/test//<lane>` in place of the two directories |
| the user's Jamfiles | `python3 tools/test/crypto_jamfiles_test.py` |
| the lint | `tools/lint/lint.sh --clang-format .local/wasi-sdk/bin/clang-format --clang-tidy .local/wasi-sdk/bin/clang-tidy`, last line `lint: clean` |

Only a build from scratch (`-a`) counts as evidence.

## What to verify first

1. `git -C <superproject> log --format='%an %ae %cn %ce'` on the base: every commit authored and
   committed by `R`, and `f7c96d3` not an ancestor (the plan's identity check prints
   `identity: ok`).
2. `webcpp::test::exit_status` exists on the base (`tools/node-test/include/webcpp/test/exit_status.hpp`).
3. After Task 1: `python3 tools/test/jamroot_test.py test_third_party_has_both_ids` passes, and
   `libs/crypto/test//config` builds with and without exceptions.
4. After Task 7: the 18 cases of the GCM specification pass on BearSSL on native and wasip2.

## Known issues

- **The plan is a draft** and was written on one machine (macOS arm64); the Linux, Windows and
  arm64 counts are the CI's to establish.
- **Plan departures for the owner (O5):** C17 (sizes at b2's `speed` and `space`), C16 (webcpp's
  own throughput program in place of a workflow no tree holds), C14 (MSVC's messages checked in
  the tools test), and `brew install botan`.
- **The CI has no level, no Linux arm64 lane and no "without exceptions" lane** at the plan's
  base; the plan builds the nightly level of the own lanes (Task 25b) and declares exception-off
  twins itself (`crypto-run`).
- **Botan has no hook** for its allocations (3.13.0), and no Botan exists for WebAssembly: both
  are recorded limits, not defects.
- **Not carried:** the controller's ledger and the independent reviews of the specification
  (findings I1 to I8, M1 to M14, and the second review's C1, I1 to I5, M1 to M12), private notes
  of the original machine; the spec cites their rulings and the plan records each in its
  "Conflicts with the spec" table and "Review Focus".

## Decisions pending

- The base: the owner's rewrite of `main`'s history, then Task 0a.
- G1 and G2: the repository `webcpporg/crypto` and its submodule.
- O2 (nightly level), O3 (NIST's archives), O5 (C14, C16, C17, Botan on the development machine).
- Whether `webcpp::webrtc` and `webcpp::beast` adopt the library.
