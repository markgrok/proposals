# Tasks

Each task points to its section in `reference/plan.md`, which holds its files, interfaces,
steps, code and expected output. Nothing is built: the library's repository has no commit, so
every task is open. The plan's tasks are numbered 0a, 0, 1 to 25, 25a, 25b and 26 to 39; here
they are grouped by the spec's five increments (spec section 9). The plan's definition of
green is Task 25's Step 2, and `reference/STATUS.md` says how to start.

## 1. Increment 1: the foundation, and AES-GCM on BearSSL and OpenSSL

- [ ] 1.1 `infra/shared` integrated into `main` first, after the owner's rewrite of `main`'s history (plan: Task 0a, controller)
- [ ] 1.2 The worktree on asio's Task 2, the ledger and the baseline (plan: Task 0)
- [ ] 1.3 The library's skeleton, the second project id `/third_party` and the test Jamfile's rule `crypto-run` (plan: Task 1)
- [ ] 1.4 The errors: `error`, its category, values 1 to 14 (plan: Task 2)
- [ ] 1.5 `equal`, and the wipe (plan: Task 3)
- [ ] 1.6 `random_device` and the entropy of each target (plan: Task 4)
- [ ] 1.7 BearSSL from the mirror, file by file, with its canary (plan: Task 5)
- [ ] 1.8 The configuration's refusals, the default backend, and BearSSL's three layers (plan: Task 6)
- [ ] 1.9 The block cipher's shape, and GCM one shot on BearSSL (plan: Task 7)
- [ ] 1.10 The incremental seal, and the limits (plan: Task 8)
- [ ] 1.11 Wycheproof and NIST CAVP, through BearSSL (plan: Task 9)
- [ ] 1.12 The user's Jamfiles as fixtures of a test of the tools, and webcpp's own programs marked (plan: Task 10)
- [ ] 1.13 OpenSSL, found or stopping, linked shared or static (plan: Task 11)
- [ ] 1.14 The OpenSSL backend: AES and AES-GCM, and OpenSSL's error queue (plan: Task 12)
- [ ] 1.15 BearSSL and OpenSSL agree (plan: Task 13)
- [ ] 1.16 The default backend, the convenience headers, and their net on MSVC (plan: Task 14)
- [ ] 1.17 OpenSSL's memory functions, an option of the config: the adapter and the install (plan: Task 15)
- [ ] 1.18 The memory option, process by process (plan: Task 16)
- [ ] 1.19 Allocation on every backend, and BearSSL's seeder (plan: Task 17)
- [ ] 1.20 Every message at link time and on MSVC, and the canary's pins (plan: Task 18)
- [ ] 1.21 `<windows.h>` before and after every header, on MSVC (plan: Task 19)
- [ ] 1.22 Constant time: Valgrind, MemorySanitizer and dudect (plan: Task 20)
- [ ] 1.23 A library named and not called adds no byte: `webcpp.same-program` (plan: Task 21)
- [ ] 1.24 Sizes and throughput (plan: Task 22)
- [ ] 1.25 The browser: WebCrypto as an outside oracle, the audio worklet, throughput in Chrome (plan: Task 23)
- [ ] 1.26 The documentation: the page, the user's Jamfiles, the examples (plan: Task 24)
- [ ] 1.27 Increment 1 whole: the rulebook, green, review and the gates (plan: Task 25)
- [ ] 1.28 asio's OpenSSL onto `/third_party//openssl` (plan: Task 25a; waits for asio's Task 21)
- [ ] 1.29 The nightly level: the heavy own lanes, asio's asyncify, the tools' macOS and Windows legs (plan: Task 25b; owner gate O2)

## 2. Increment 2: libsecp256k1 and ctaes

- [ ] 2.1 libsecp256k1 built file by file, its tables feature, its canary and its own tests (plan: Task 26)
- [ ] 2.2 `secp256k1::keypair`, `verify_schnorr` and `verify_ecdsa` (plan: Task 27)
- [ ] 2.3 secp256k1's vectors: BIP340, Wycheproof's Bitcoin ECDSA, and ECDH (plan: Task 28)
- [ ] 2.4 ctaes, Bitcoin Core's copy: the block cipher alone (plan: Task 29)
- [ ] 2.5 Increment 2 whole: green, review (plan: Task 30)

## 3. Increment 3: trystero's migration

- [ ] 3.1 trystero seals with `aes256_gcm` directly, and draws from `random_device` (plan: Task 31; starts after trystero's shared-connection change has integrated)
- [ ] 3.2 trystero signs with `secp256k1::keypair`, and libsecp256k1 comes from `third_party` (plan: Task 32)
- [ ] 3.3 The memory option beside Boost.Asio's SSL, libdatachannel and a TLS handshake (plan: Task 33)
- [ ] 3.4 Increment 3 whole: green, review, the gates (plan: Task 34)

## 4. Increment 4: ChaCha20-Poly1305

- [ ] 4.1 ChaCha20-Poly1305 on BearSSL and OpenSSL, with its vectors, its cross-backend lane and its part of the memory option's tests (plan: Task 35)
- [ ] 4.2 Increment 4 whole: green, review, push (plan: Task 36)

## 5. Increment 5: Botan, native, with exceptions

- [ ] 5.1 Botan found, or the build stops (plan: Task 37; owner item O5, `brew install botan`)
- [ ] 5.2 The Botan backend, with exceptions only (plan: Task 38)
- [ ] 5.3 Botan in every native lane, its page, and the last green (plan: Task 39)

## 6. Publication

- [ ] 6.1 The owner creates `webcpporg/crypto`, public, BSL-1.0, and adds it as the submodule `libs/crypto` (plan: Global Constraints, gates G1 and G2)
- [ ] 6.2 Pushes of the library, trystero and the superproject, each with the owner's agreement (plan: gate G4, Tasks 25, 30, 34 and 36)
