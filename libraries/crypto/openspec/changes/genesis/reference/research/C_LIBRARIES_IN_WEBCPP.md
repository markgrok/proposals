---
title: "C libraries in webcpp: BearSSL, libsecp256k1 and OpenSSL as webcpp::crypto backends"
tags: [webcpp, crypto, bearssl, openssl, secp256k1, b2, wasm, emscripten, wasi, build]
status: active
created: 2026-10-10
---

# C libraries in webcpp: BearSSL, libsecp256k1 and OpenSSL as webcpp::crypto backends

Measured research for the owner's decision "2026-10-10, owner: webcpp::crypto's backends are the C
libraries themselves" (`~/.buzz/PLANS/WEBCPP_DESIGN.md`, last entries). Every number below was
measured on 2026-10-10/11 on the owner's machine; nothing is quoted from memory.

## Setup

| Item | Value |
| --- | --- |
| Host | Apple M5 Max, arm64, macOS (Darwin 27.0.0). No x86 host, no Rosetta (`arch -x86_64` fails: "Bad CPU type") |
| Native compiler | Apple clang 21.0.0 (arm64; `-arch x86_64` cross-compiles and links, but cannot run) |
| WASI | wasi-sdk 34.0 (LLVM 23.1.0), `--target=wasm32-wasip2` / `wasm32-wasip3`, run by wasmtime 47.0.3 (`-W component-model-async=y -S p3=y` for p3) |
| Emscripten | 6.0.11, `EM_CACHE=~/dev/webcpp/.local/emscripten-cache`, run by node 26.7.0 |
| OpenSSL | Homebrew openssl@3, 3.6.4 (25 Aug 2026): `libcrypto.3.dylib` 4.87 MB and `libcrypto.a` 8.58 MB |
| b2 | B2 5.5.3 (`/opt/homebrew/bin/b2`), with `~/dev/webcpp/.local/user-config.jam` |
| BearSSL | `https://www.bearssl.org/git/BearSSL`, commit `7bea48e5e850ab4cafbe68d3765cdaba13a86d6f` (2026-04-06, "Fixed bug in handling incoming records with invalid length"), 294 `.c` files under `src/` |
| BearSSL licence | MIT, `LICENSE.txt`: "Copyright (c) 2016 Thomas Pornin <pornin@bolet.org>. Permission is hereby granted, free of charge, ..." (the standard MIT text) |
| libsecp256k1 | `https://github.com/bitcoin-core/secp256k1`, tag `v0.8.0` = commit `6e2c8bc4ecdc6e71dbe7a368f360d8d453ce435d` (2026-08-03); the same version as `.local/secp256k1-native` (its `.pc` says 0.8.0) |
| libsecp256k1 licence | MIT, `COPYING`: "Copyright (c) 2013 Pieter Wuille. Permission is hereby granted, ..." |

All probes live in `~/dev/webcpp/.superpowers/sdd/plan-reviews/c-backends/`:
`probe/cc.sh` (one compiler per target), `probe/bearssl/` (unity sources), `probe/tools/`
(`statics.py`, `extract_all.sh`, `amalgamate.py`), `probe/bench/` (programs), `probe/secp256k1/`,
`probe/config/` (the config sketch), `probe/b2/Jamroot` (the b2 prototype), logs in `out/`.
No repository was modified.

## 1. BearSSL as one translation unit

### 1a. As C, naively (`#include` every `.c` file)

`probe/bearssl/all.c`: `#define BR_ENABLE_INTRINSICS 1` then `#include` of all 294 files.
`probe/bearssl/aead.c`: the 30 files of AES-GCM (aes_ct, aes_ct64, aes_x86ni, aes_pwr8 with their
CTR, ghash_ctmul/ctmul32/ctmul64/pclmul/pwr8, gcm, sha2small, hmac, hmac_drbg, sysrng, codecs).

    probe/cc.sh <target> c -std=c99 -O2 -Wall -I bearssl/inc -I bearssl/src -c all.c

| Unit | native arm64 | x86_64 | wasip2 | wasip3 | emscripten |
| --- | --- | --- | --- | --- | --- |
| all 294 files | fails, 575 errors | fails | fails, 877 errors | fails, 877 | fails |
| AEAD subset (30) | fails, 7 errors | fails, 7 | fails, 16 | fails, 16 | fails, 16 |

Errors met (with `-ferror-limit=0`):

- **Static names colliding**: 132 distinct names redefined across files, for example `Rcon`,
  `sub_word`, `add_round_key`, `shift_rows`, `mix_columns`, `xorbuf` (aes_ct vs aes_ct64),
  `api_generator`, `api_order`, `api_mul`, ... (every `ec_*.c` defines the same `api_*` set),
  `C255_P`, `GEN`, `ORDER`, `cswap`, `byteswap`.
- **File-local types colliding**: `p256_jacobian` is a different struct in `ec_p256_m15/m31/m62/m64.c`
  ("typedef redefinition with different types").
- **File-local macros leaking** into later files (`P0I` redefined, then cascades: "expression is not
  assignable", "expected identifier", "use of undeclared identifier 'base'").
- wasi-sdk's clang 23 makes `-Wincompatible-pointer-types` an error by default, Apple clang 21 a
  warning, hence the different counts; the cause is the same collisions.
- Two switches of `inner.h` are read only at its first inclusion (include guard): `BR_ENABLE_INTRINSICS`
  (x86 intrinsics headers and `br_cpuid`) and `BR_POWER_ASM_MACROS` (the POWER8 asm macros). In one unit
  the first file decides for all, so both must be defined before any source.

**A collision-free hand-picked subset works without any change.** `aes_common.c` (only used by
aes_big/aes_small) also defines `Rcon`; dropping it and the 32-bit aes_ct/ghash_ctmul files leaves 24
files (`probe/bearssl/min.c`: gcm, aes_ct64 x3, aes_x86ni x2, aes_pwr8 x2, ghash_ctmul64,
ghash_pclmul, ghash_pwr8, sha2small, hmac, hmac_drbg, sysrng, 9 codec files) that compile with
**0 errors and 0 warnings (`-Wall -Wextra`) on native, x86_64, wasip2, wasip3 and emscripten**.

**The whole library works through a generated amalgamation that renames, without touching a source.**
`probe/tools/statics.py` reads clang's JSON AST of each file and lists what the file itself defines at
file scope with internal linkage (static functions and variables), its own types and enumerators, and
the macros it `#define`s. `extract_all.sh` runs it under four configurations (arm64, x86_64 with
intrinsics, wasm32, ppc64le POWER8, the last through wasi-sdk's front end with `-fsyntax-only` headers)
so that names inside `#if BR_AES_X86NI` / `#if BR_POWER8` are found too. `amalgamate.py` then writes
`all_renamed.c`:

    /* symcipher/aes_ct64.c */
    #define Rcon br_unity_symcipher_aes_ct64_Rcon
    ...
    #include "symcipher/aes_ct64.c"
    #undef Rcon
    ... (#undef of every macro the file defined)

It defines `BR_ENABLE_INTRINSICS 1` and `BR_POWER_ASM_MACROS 1` once at the top. 668 names are renamed
across 294 files. Results:

| Unit (as C, gnu dialect, `-O2 -Wall`) | native | x86_64 | wasip2 | wasip3 | emscripten | ppc64le (syntax) |
| --- | --- | --- | --- | --- | --- | --- |
| `all_renamed.c` (294 files) | 0 err, 0 warn | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 errors |
| `aead_renamed.c` (30 files) | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | 0 / 0 | - |

Verification that the unit is BearSSL and nothing else:

- **Exported symbols identical**: the 294 files compiled one by one export 527 global symbols; the
  amalgamation exports the same 527 (`diff` empty). The 374 renamed `br_unity_*` symbols are all local.
- **BearSSL's own test suite passes on it**: `test/test_crypto.c all` linked against the amalgamation
  object passes all 88 tests natively (27.8 s), on wasip2 under wasmtime (39.5 s) and on Emscripten under
  node (45.9 s). The x86ni, pwr8, pclmul and sse2 tests report "UNAVAILABLE" on this ARM host.
- **No size cost from including everything**: linked with `--gc-sections` (wasm) or `-dead_strip`
  (macOS), the AES-GCM program is the same size with all of BearSSL as with the 24-file subset
  (emscripten -O2 32,508 vs 32,519 bytes; wasip2 -Os stripped 60,570 vs 60,575; native -O2 53,024 vs
  52,896).

Two more findings:

- **POWER8 needs GNU C**: with `-std=c99` the POWER8 files fail ("use of undeclared identifier 'asm'",
  20 errors); the unit must be compiled in the compiler's default GNU dialect (BearSSL's own
  `conf/Unix.mk` passes no `-std`).
- **Missing randomness on WASI and Emscripten** (no header is missing; the RNG is): see section 2.
  `getentropy()` exists in wasi-libc (`include/wasm32-wasip2/unistd.h:261`) and Emscripten's
  `unistd.h:193`, but only under `_GNU_SOURCE`/`_BSD_SOURCE`, so the unit needs `_DEFAULT_SOURCE` and
  `BR_USE_GETENTROPY=1` there. With `-std=c99` and only `BR_USE_GETENTROPY=1` it fails: "call to
  undeclared function 'getentropy'". Putting both defines at the top of `src.c` under
  `#if defined(__wasi__) || defined(__EMSCRIPTEN__)` works with no command-line flag on all four
  targets (`probe/bearssl/src.c`, run on each).

### 1b. As C++ (a `src.hpp` included in a `.cpp`, Boost.JSON style)

**Impossible without editing BearSSL.** Every one of the 294 files, compiled alone as C++
(`clang++ -x c++ -std=c++20 -fsyntax-only`), fails: 3,722 errors, 3,701 of them "assigning to
'unsigned char *' from incompatible type 'void *'" in `inner.h` itself (lines 597, 613, 627, 641, 655:
the `br_enc*`/`br_dec*` inline helpers), the rest the same C idiom in `pemdec.c`, `ec_pubkey.c` and the
RSA OAEP/PSS files ("no matching function" through `void *`). Wrapped in `extern "C" { }` the renamed
amalgamation still gives 206 errors (all) and 41 (AEAD subset), identical on native, wasip2 and
emscripten. Clang has no switch that accepts implicit `void *` conversions in C++. A C++ unit would
require a port, which the owner refused.

## 2. What BearSSL picks at run time

| Path | Compile-time gate (`src/inner.h`) | Run-time check | Selected by |
| --- | --- | --- | --- |
| AES-NI (`aes_x86ni*`) | `BR_AES_X86NI`: `(BR_i386 \|\| BR_amd64) && (GCC 4.8+ \|\| clang 3.7+ \|\| MSVC 2012+)` | `br_aes_x86ni_supported()`: `br_cpuid(0,0,0x02080000,0)`, CPUID leaf 1 ECX bit 25 (AES-NI) and bit 19 (SSE4.1) | `br_aes_x86ni_ctr_get_vtable()` returns the vtable or NULL |
| PCLMULQDQ (`ghash_pclmul`) | same as AES-NI | `br_cpuid(0,0,0x00000002,0)`, ECX bit 1 | `br_ghash_pclmul_get()` returns the function or 0 |
| RDRAND seeder | `BR_RDRAND` (x86) | ECX bit 30 | `br_prng_seeder_system()` |
| POWER8 (`aes_pwr8`, `ghash_pwr8`) | `BR_POWER8`: `__GNUC__ && (_ARCH_PWR8 \|\| _ARCH_PPC) && __CRYPTO__` | none: `br_aes_pwr8_supported()` returns 1 | `br_aes_pwr8_ctr_get_vtable()`, `br_ghash_pwr8_get()` |
| ARMv8 AES/PMULL, wasm SIMD | **none exists**: no `__ARM_FEATURE_CRYPTO`, `arm_neon`, `vaeseq` or `__wasm` anywhere in `src/` (grep count 0) | - | - |

The intrinsics functions carry `__attribute__((target("sse2,aes,pclmul")))` (`BR_TARGET`), so an
ordinary x86-64 build (no `-maes`) contains them and runs on any x86 CPU; the CPUID test chooses. Measured
on the x86_64 cross-build of the benchmark (`-arch x86_64 -O2`): the binary holds 68 `aesenc/aesenclast`,
35 `pclmulqdq` and 6 `cpuid` instructions and the symbols `br_aes_x86ni_ctr_run`, `br_ghash_pclmul`.
The caller picks: `ssl/ssl_engine_default_aesgcm.c` tries x86ni, then pwr8, then `aes_ct64` if `BR_64`
else `aes_ct`, and for GHASH pclmul, pwr8, then `ctmul32` (`BR_LOMUL`), `ctmul64` (`BR_64`) or `ctmul`.

**On wasm** neither gate is defined, so the `*_get_vtable()`/`*_get()` functions compile to stubs that
return NULL/0 and only the constant-time software code remains. Two traps measured:

- `BR_64` is **not** set on wasm32 (`ULONG_MAX` is 32-bit and no 64-bit architecture macro matches),
  so BearSSL's own defaults pick the 32-bit `aes_ct` and `ghash_ctmul`. But 64-bit integers are native
  in wasm: `aes_ct64` + `ghash_ctmul64` run at 159 MiB/s against 79-80 MiB/s for `aes_ct` + `ghash_ctmul32`
  (table in section 3). webcpp's backend must name `br_aes_ct64_ctr_vtable` and `br_ghash_ctmul64`
  itself on wasm rather than rely on BearSSL's defaults.
- `br_prng_seeder_system()` returns 0 with the name "none" on wasip2, wasip3 and Emscripten (the
  benchmark printed "no system seeder (none)" on all three): `inner.h` enables `/dev/urandom` and
  `getentropy` only for listed OSes (Linux, BSDs, macOS, ...), and WASI and Emscripten are not among
  them. Fixed by `_DEFAULT_SOURCE` + `BR_USE_GETENTROPY=1` (section 1a); the benchmark then reports
  `seeder=getentropy` on all three. Natively on macOS it reports `seeder=urandom`.

## 3. Sizes and speed

Program: `probe/bench/seal_bearssl.c` seeds an HMAC-DRBG from the system seeder, draws a key, an IV
and 1 MiB, then N times seals the 1 MiB with AES-256-GCM and opens it again, checking tag and data;
it picks x86ni/pwr8 when available, else ct64. `seal_openssl.c` does the same through EVP
(`EVP_aes_256_gcm`, `RAND_bytes`). `baseline.c` makes the same libc calls (malloc, clock_gettime,
printf) without cryptography, to show the library's own share. All built with
`-ffunction-sections -fdata-sections` and `-Wl,--gc-sections` (wasm) or `-Wl,-dead_strip` (macOS).
WASI files marked "stripped" were linked with `-Wl,--strip-all`; Emscripten strips by itself at -O2/-Os.

### Sizes (bytes; gzip is `gzip -9`)

| Target | Build | Baseline raw / gzip | BearSSL AES-GCM raw / gzip | BearSSL's share raw / gzip |
| --- | --- | --- | --- | --- |
| emscripten (.wasm) | -O2 | 9,392 / 5,021 | 32,519 / 14,029 | +23,127 / +9,008 |
| emscripten (.wasm) | -Os | 8,779 / 4,782 | 27,035 / 12,751 | +18,256 / +7,969 |
| emscripten (.js glue) | -O2 / -Os | 10,604 / 10,135 | 11,325 / 10,679 | +721 / +544 |
| wasip2 component, stripped | -O2 | 44,264 / 16,663 | 65,286 / 24,090 | +21,022 / +7,427 |
| wasip2 component, stripped | -Os | 44,264 / 16,663 | 60,575 / 23,062 | +16,311 / +6,399 |
| wasip3 component, stripped | -O2 | 52,188 / 19,354 | 73,273 / 26,756 | +21,085 / +7,402 |
| wasip3 component, stripped | -Os | 52,188 / 19,354 | 68,562 / 25,800 | +16,374 / +6,446 |
| wasip2 component, not stripped | -O2 / -Os | 279,635 / 94,638 | 305,398 / 300,796 | names and debug sections dominate |
| wasip3 component, not stripped | -O2 / -Os | 331,434 / 112,462 | 357,427 / 352,825 | idem |
| native arm64 `__text` | -O2 / -Os | 112 | 14,340 / 11,808 | about 14 KB / 12 KB of code |
| native arm64 file | -O2 / -Os | 33,536 | 52,896 / 36,624 | (Mach-O pages) |

OpenSSL natively, the same program: linked shared, the executable is 34,496 bytes plus the system's
`libcrypto.3.dylib` (4.87 MB); linked static (`libcrypto.a`, dead-stripped) it is **3,619,232 bytes**
(`__text` 1,542,468, `__const` 882,656): EVP's provider machinery comes in whole. No OpenSSL for WASI or
Emscripten exists in `.local/`; none was built.

Without `--gc-sections`/`-dead_strip` the size is lost: the same program built by b2's default
`variant=release` (no section GC on clang-darwin) is 482,256 bytes natively. webcpp's build must pass
`-ffunction-sections -fdata-sections` to the C unit and dead-strip at link.

### Throughput (MiB/s, seal + open of 1 MiB counted as 2 MiB; Apple M5 Max; 3 runs each)

| Target | Code path | MiB/s |
| --- | --- | --- |
| native arm64, OpenSSL 3.6.4 shared | ARMv8 AES + PMULL (OpenSSL's assembly) | 7,501 - 7,687 |
| native arm64, OpenSSL 3.6.4 static | idem | 7,546 - 7,645 |
| native arm64, BearSSL -O2/-Os, subset or whole | aes_ct64 + ghash_ctmul64 (no ARM hardware path exists) | 159 - 175 |
| native arm64, BearSSL | aes_ct + ghash_ctmul32 | 91 - 96 |
| wasip2 (wasmtime 47), BearSSL -O2 | aes_ct64 + ctmul64 | 159.4 - 159.7 |
| wasip3 (wasmtime 47), BearSSL -O2 | aes_ct64 + ctmul64 | 159.1 - 159.5 |
| emscripten (node 26.7), BearSSL -O2 | aes_ct64 + ctmul64 | 158.6 - 160.7 |
| wasip2 / emscripten, BearSSL -Os | aes_ct64 + ctmul64 | 146 - 148 / 145 - 146 |
| wasip2 / emscripten, BearSSL | aes_ct + ctmul32 | 78 - 79 / 78 - 80 |

So on this ARM server-class CPU OpenSSL is about **46 times** faster than BearSSL, because BearSSL has
no ARMv8 crypto-extension code. **BearSSL's AES-NI path could not be measured here** (no x86 host and
no Rosetta): it is present and selected by CPUID (section 2) but its x86 speed against OpenSSL's
AES-NI/VAES must be measured on an x86-64 runner (the CI's ubuntu-24.04 runner is x86-64; BearSSL's
`test/test_speed.c` or this benchmark serve). Wasm runs the constant-time code at native speed (159 vs
159 - 175 MiB/s), and wasm has no AES instruction, so on the web the choice is between this and the
browser's WebCrypto, not between BearSSL and OpenSSL.

## 4. libsecp256k1 as one unit

`probe/secp256k1/src.c` defines the six module switches (`ENABLE_MODULE_ECDH`, `RECOVERY`,
`EXTRAKEYS`, `SCHNORRSIG`, `MUSIG`, `ELLSWIFT`, plus `SILENTPAYMENTS`, which v0.8.0 has) and includes
`secp256k1.c`, `precomputed_ecmult.c`, `precomputed_ecmult_gen.c` (the three files of its CMake build).

| Mode | native | x86_64 | wasip2 | wasip3 | emscripten |
| --- | --- | --- | --- | --- | --- |
| C (`-std=c89 -O2 -Wall`) | builds, 11 warnings | builds | builds | builds | builds |
| C++ (`extern "C"`, `-std=c++20`) | 10 errors | 10 | 10 | 10 | 10 |
| C, used by a program (key pair, BIP340 sign/verify, ECDH) | runs | links | runs (wasmtime) | runs | runs (node) |
| via b2 `lib secp256k1 : src.c : <link>static` | runs | - | runs | runs | runs |

The 11 warnings are all `-Wunused-function` (static helpers a unity build does not call). The C++
failure is deliberate: `src/secp256k1.c:14-15` reads
`#if defined(__cplusplus) && !defined(SECP256K1_CPLUSPLUS_TEST_OVERRIDE)` /
`#error Trying to compile a C project with a C++ compiler.`, and past it come `void *` conversions
(`util.h:282`, `secp256k1.c:145`, ...).

Size and speed depend on the precomputed tables:

| Configuration | emscripten -Os wasm raw / gzip | wasip2 -Os stripped raw / gzip | native verify/s | wasip2 verify/s |
| --- | --- | --- | --- | --- |
| defaults (`ECMULT_WINDOW_SIZE` 15: `secp256k1_pre_g` and `pre_g_128`, 512 KiB each) | 1,211,371 / 1,125,541 | 1,256,034 / 1,137,008 (not stripped) | 50,289 | 2,937 |
| `ECMULT_WINDOW_SIZE=2`, `COMB_BLOCKS=2`, `COMB_TEETH=5` | 141,929 / 56,007 | 187,107 / 66,677 | 33,978 | 1,744 |

Emscripten with the defaults verified 3,875 signatures/s under node. Forcing 128-bit field arithmetic
on wasm32 (`USE_FORCE_WIDEMUL_INT128`) gave 3,029/s, no real gain. The table size is the web/server
knob: about 1.07 MB less for a third slower verification.

What it exports (v0.8.0, the public headers `include/secp256k1*.h`; 82 functions, all present as
global symbols of the unit):

| Header (module) | Functions |
| --- | --- |
| `secp256k1.h` (core) | context_create, context_clone, context_destroy, context_randomize, context_set_illegal_callback, context_set_error_callback, context_set_sha256_compression, selftest, ec_pubkey_parse, ec_pubkey_serialize, ec_pubkey_cmp, ec_pubkey_sort, ec_pubkey_create, ec_pubkey_negate, ec_pubkey_tweak_add, ec_pubkey_tweak_mul, ec_pubkey_combine, ec_seckey_verify, ec_seckey_negate, ec_seckey_tweak_add, ec_seckey_tweak_mul, ecdsa_sign, ecdsa_verify, ecdsa_signature_parse_der, ecdsa_signature_parse_compact, ecdsa_signature_serialize_der, ecdsa_signature_serialize_compact, ecdsa_signature_normalize, tagged_sha256; data: context_static, nonce_function_rfc6979, nonce_function_default |
| `secp256k1_preallocated.h` | context_preallocated_size, context_preallocated_create, context_preallocated_clone_size, context_preallocated_clone, context_preallocated_destroy |
| `secp256k1_ecdh.h` (ecdh) | ecdh; data: ecdh_hash_function_sha256, ecdh_hash_function_default |
| `secp256k1_recovery.h` (recovery) | ecdsa_sign_recoverable, ecdsa_recover, ecdsa_recoverable_signature_parse_compact, ecdsa_recoverable_signature_serialize_compact, ecdsa_recoverable_signature_convert |
| `secp256k1_extrakeys.h` (extrakeys) | xonly_pubkey_parse, xonly_pubkey_serialize, xonly_pubkey_cmp, xonly_pubkey_from_pubkey, xonly_pubkey_tweak_add, xonly_pubkey_tweak_add_check, keypair_create, keypair_sec, keypair_pub, keypair_xonly_pub, keypair_xonly_tweak_add |
| `secp256k1_schnorrsig.h` (schnorrsig) | schnorrsig_sign32, schnorrsig_sign_custom, schnorrsig_verify; data: nonce_function_bip340 |
| `secp256k1_ellswift.h` (ellswift) | ellswift_encode, ellswift_decode, ellswift_create, ellswift_xdh; data: ellswift_xdh_hash_function_prefix, ellswift_xdh_hash_function_bip324 |
| `secp256k1_musig.h` (musig) | musig_pubnonce_parse, musig_pubnonce_serialize, musig_aggnonce_parse, musig_aggnonce_serialize, musig_partial_sig_parse, musig_partial_sig_serialize, musig_pubkey_agg, musig_pubkey_get, musig_pubkey_ec_tweak_add, musig_pubkey_xonly_tweak_add, musig_nonce_gen, musig_nonce_gen_counter, musig_nonce_agg, musig_nonce_process, musig_partial_sign, musig_partial_sig_verify, musig_partial_sig_agg |
| `secp256k1_silentpayments.h` (silentpayments) | silentpayments_sender_create_outputs, silentpayments_recipient_label_create, silentpayments_recipient_create_labeled_spend_pubkey, silentpayments_recipient_prevouts_summary_create, silentpayments_recipient_scan_outputs, silentpayments_recipient_label_serialize, silentpayments_recipient_label_parse |

(every name prefixed `secp256k1_`). The unit also exports the tables `secp256k1_pre_g`,
`secp256k1_pre_g_128` and `secp256k1_ecmult_gen_prec_table` as global symbols: a program must never
link webcpp's unit together with another libsecp256k1 (trystero's `.local/secp256k1-*` today), or the
link fails on duplicates. trystero must move to the backend in the same change.

## 5. OpenSSL static or shared through b2

Prototype `probe/b2/Jamroot`, built and run:

    feature.feature webcpp-crypto-openssl-link : shared static : propagated ;
    lib crypto : : <file>$(openssl-root)/lib/libcrypto.a <webcpp-crypto-openssl-link>static
      : : <include>$(openssl-root)/include <define>WEBCPP_CRYPTO_HAS_OPENSSL ;
    lib crypto : : <name>crypto <search>$(openssl-root)/lib <webcpp-crypto-openssl-link>shared
      : : <include>$(openssl-root)/include <define>WEBCPP_CRYPTO_HAS_OPENSSL
        <define>WEBCPP_CRYPTO_OPENSSL_SHARED ;

| Command | Result |
| --- | --- |
| `b2 toolset=clang variant=release seal_openssl` | `bin/clang-darwin-21/release/seal_openssl`, 34,480 bytes, `otool -L` shows `libcrypto.3.dylib` |
| `b2 ... webcpp-crypto-openssl-link=static seal_openssl` | `bin/clang-darwin-21/release/webcpp-crypto-openssl-link-static/seal_openssl`, 4,384,720 bytes, no libcrypto dependency |

The feature is propagated (a program's request reaches the library) and not free or incidental (the
two builds get separate directories, as above). `shared` first makes it the default. In webcpp it
goes beside the existing finder: `webcpp.external crypto : OpenSSL : OPENSSL_ROOT : openssl :
openssl/evp.h : native` returns the root both alternatives search, and a `<conditional>` like
trystero's `native-dependencies` adds `<library>crypto` and the define only when OpenSSL was found, so
that without OpenSSL the library still builds, on BearSSL (not prototyped: the `<conditional>` part).
Per platform: the MSVC names are `libcrypto.lib` in `lib/VC/x64/MD` (as trystero's `build.jam` already
says); a static OpenSSL needs `ws2_32 crypt32 user32 advapi32` on Windows and `-pthread` (and `-ldl`
on older glibc) on Linux; Emscripten and WASI have no shared libraries in scope (WASI components have
no `dlopen`), so the feature only exists on native.

What a header can and cannot decide:

- It **cannot** change how anything is linked: linking happens after every header has been read. (The one
  exception is MSVC's `#pragma comment(lib, "...")`, Boost's auto-link, which can name a library but
  not choose static versus shared beyond its name; no other toolset has it.)
- It **can** read what the build tells it (`WEBCPP_CRYPTO_HAS_OPENSSL`, `WEBCPP_CRYPTO_OPENSSL_SHARED`,
  usage requirements of the `crypto` target) and what the compiler tells it (`__EMSCRIPTEN__`,
  `__wasi__`, `__x86_64__`, `__aarch64__`, `__powerpc64__`), refuse impossible combinations with
  `#error` or `static_assert`, and pick a default backend.

## 6. Metaprogramming the default

Sketch `probe/config/include/webcpp/crypto/config.hpp` (about 90 lines): it classifies the target
(emscripten / wasi / native) and the architecture; `#error`s on `WEBCPP_CRYPTO_HAS_OPENSSL` on a
WebAssembly target, on `WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL` without OpenSSL, on two default
backends, and on `WEBCPP_CRYPTO_OPENSSL_SHARED` without OpenSSL; declares `backends::bearssl` and
`backends::openssl`; `is_available<Backend>` (true for bearssl always, for openssl only with
`WEBCPP_CRYPTO_HAS_OPENSSL`); and

    #if defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL)
    using default_backend = backends::bearssl;
    #elif defined(WEBCPP_CRYPTO_DEFAULT_BACKEND_OPENSSL) || defined(WEBCPP_CRYPTO_HAS_OPENSSL)
    using default_backend = backends::openssl;
    #else
    using default_backend = backends::bearssl;
    #endif

    template <class Backend = default_backend> class basic_aes256_gcm {
        static_assert(is_available_v<Backend>,
            "webcpp::crypto: this backend is not available on this target or in this build");
        ...
    };
    using aes256_gcm = basic_aes256_gcm<>;

Checked with stub traits (`stub_traits.hpp`, `use.cpp`, `fail_openssl.cpp`), C++20:

| Check | native | wasip2 | wasip3 | emscripten |
| --- | --- | --- | --- | --- |
| `use.cpp` builds and runs, default backend printed | bearssl; `openssl` with `-DWEBCPP_CRYPTO_HAS_OPENSSL` | bearssl | bearssl | bearssl |
| `basic_aes256_gcm<backends::openssl>` without OpenSSL | static_assert fires | fires | fires | fires |
| `-DWEBCPP_CRYPTO_HAS_OPENSSL` | accepted | `#error ... OpenSSL is native only` | `#error` | `#error` |

The architecture does not need to enter the type: BearSSL chooses AES-NI/PCLMUL by CPUID inside its own
code, so `backends::bearssl` is one type on every target. A finer choice (BearSSL ct64 against ct on a
32-bit native CPU) belongs inside the backend, not in the default.

## 7. Where the C sources live

| | Submodule under `third_party/libs/<name>`, pinned | Fetch at configure time |
| --- | --- | --- |
| Precedent in webcpp | branch `lib/regex`: `third_party/libs/srell` (url `../srell.git`, commit `0b95517`) and `third_party/libs/JSON-Schema-Test-Suite` (`5b0ee16`), declared in `third_party/Jamfile` as `/webcpp/third_party//<target>` with a `<conditional>` that stops the build, naming `git submodule update --init third_party/libs/<name>`; `AGENTS.md` "Third-party code" makes it the rule | none in webcpp (an earlier private project has `./configure` with pins and SHA-256, a different project) |
| Pin | the commit is in the superproject's tree; moving it is a reviewed change | a pin file plus a hash, checked by a script |
| CI | `actions/checkout` with `submodules: recursive`, which `library.yml` already does; any CI with git works | needs network during the build and a configure step b2 does not have today |
| Offline and archives | a clone or a release archive holds everything; b2 alone builds | the first build needs the network; an archive must bundle the fetch |
| Risk | BearSSL's upstream is `bearssl.org/git`, not GitHub: a recursive checkout then depends on that server. A plain mirror `webcpporg/bearssl` (no patches) removes it; the rule today keeps mirrors for patched code, so this is an owner decision | the same server dependency, at every cold build |

The submodule keeps "any CI can build it" true with what webcpp's CI already does; fetching adds a
network dependency and a configure step to every build.

A user outside the superproject, since the unit is C and cannot be included in a `.cpp`:

- **CMake**: `add_library(webcpp_bearssl STATIC <webcpp>/libs/crypto/src/bearssl.c)`,
  `target_include_directories(webcpp_bearssl PRIVATE <bearssl>/src PUBLIC <bearssl>/inc)`,
  `target_compile_options(webcpp_bearssl PRIVATE -ffunction-sections -fdata-sections)`, then link it.
- **Plain compiler**: `cc -O2 -c bearssl.c -I<bearssl>/inc -I<bearssl>/src && c++ main.cpp bearssl.o`.

In both, `<bearssl>` must be BearSSL at the pinned commit (the submodule, or a checkout of the same
commit), because the generated renames are specific to it; the generated file names that commit in its
first comment.

## Recommendation

(Part (a) and the guideline (b) below are superseded by the owner's decision of 2026-10-11, no
`src.c`: see section 8, "Built from sources, named by the user", at the end.)

**(a) Mechanism: a `src.c`, compiled as C by a b2 static library.** Boost.JSON's `src.hpp` cannot be
copied literally, because neither library compiles as C++ (BearSSL: 3,722 errors from `inner.h`'s
implicit `void *`; libsecp256k1: a deliberate `#error`). The faithful equivalent is webcpp's own
`src.c` per library: it sets what the library cannot detect on webcpp's targets (`_DEFAULT_SOURCE` and
`BR_USE_GETENTROPY` on WASI and Emscripten), then includes the library's original `.c` files,
unchanged, in one C translation unit; b2 builds it as `lib bearssl : src/bearssl.c : <link>static`,
exactly as the Jamroot builds `tools/boost_json.cpp` into `/webcpp//boost_json`, and a program uses it
as if header-only (`<library>/webcpp/crypto//bearssl`). For libsecp256k1, `src.c` is three includes
and the module switches. For BearSSL, `src.c` is generated (the renaming amalgamation of 294 files,
668 names, measured above), because a hand-picked list breaks as soon as a second AES or curve
implementation is wanted; the generator runs only when the pin moves, and CI regenerates it and fails
on any difference. Including all of BearSSL costs nothing after section GC and lets the backend export
BearSSL's whole API. Fallback with no tooling: the 24-file subset, which compiles cleanly as is.

**(b) Guideline: how to bring a C library into webcpp** (ten lines):

1. The library is a submodule under `third_party/libs/<name>` pinned at a commit, never copied or edited; a plain `webcpporg` mirror when upstream is not on GitHub.
2. webcpp owns one `src.c` per library: target settings the library cannot detect, then `#include` of its own `.c` files. It is compiled as C, in the library's own dialect (no forced `-std`).
3. When the files collide in one unit, `src.c` is generated: each file's statics, types, enumerators and macros are renamed or undefined around its include, and both `inner.h`-style switches are set first. CI checks the committed copy.
4. b2 builds it as `lib <name> : src.c : <link>static` with `-ffunction-sections -fdata-sections`, private include of the library's `src/`, its public headers as usage requirements, on every target.
5. Proof: the library's own test suite, linked to the unit, passes on every target in CI, and the exported symbols equal the library's own build.
6. The backend is named after the library (`backends::<name>`); its documentation lists the headers and functions it exports.
7. A library webcpp does not build (OpenSSL) is found by `webcpp.external`, linked shared or static by a propagated feature, and announces itself by a define in its usage requirements.
8. The library's `config.hpp` chooses the default backend from `__EMSCRIPTEN__`, `__wasi__`, the architecture and those defines, and refuses impossible ones with `#error` or `static_assert`.
9. One copy per program: never link webcpp's unit together with a system copy of the same library.
10. Moving a pin is a change of its own, with its evidence: tests on every target and the size table.

**(c) Default backend per target.**

| Target | Default | Why (measured) |
| --- | --- | --- |
| emscripten | `backends::bearssl` (ct64) | +18 KB raw / +8 KB gzip at -Os, 146 - 160 MiB/s; OpenSSL refused at compile time. WebCrypto stays a separate, asynchronous choice outside this question |
| wasip2, wasip3 | `backends::bearssl` (ct64) | +16 KB raw / +6.4 KB gzip at -Os stripped, 147 - 160 MiB/s; the only possible backend |
| native with OpenSSL found | `backends::openssl`, linked shared by default, static on request | 7.5 GB/s against BearSSL's 0.16 GB/s on arm64 (about 46 times); static adds about 3.6 MB |
| native without OpenSSL | `backends::bearssl` | CPUID picks AES-NI/PCLMUL on x86 (present in the binary, speed still to be measured on an x86-64 runner), ct64 elsewhere |

The BearSSL backend must name `br_aes_ct64_ctr_vtable` and `br_ghash_ctmul64` itself on wasm (BearSSL's
own default there is the 32-bit code, two times slower). For libsecp256k1 the same config can choose
the tables: `ECMULT_WINDOW_SIZE=2` with comb 2x5 for the web (141,929 bytes instead of 1.21 MB on
Emscripten, a third slower verification) and the defaults on servers.

Open points for the owner: a `webcpporg/bearssl` mirror for checkout availability; measuring BearSSL's
AES-NI path against OpenSSL on the CI's x86-64 runner; and whether the generated BearSSL `src.c` lives
under `include/webcpp/crypto/` (Boost.JSON's place for `src.hpp`) or under `libs/crypto/src/`.

## AES-256-GCM throughput on CI runners (2026-10-11, run 38113681987 of webcpporg/webcpp)

Seal and open 1 MiB per round, three runs each. BearSSL is the mirror at 7bea48e5, built
with its own `make lib`, -O2; it picks AES-NI and PCLMUL at run time. OpenSSL is the
runner's.

| Runner | CPU | BearSSL | OpenSSL | OpenSSL / BearSSL |
| --- | --- | --- | --- | --- |
| ubuntu-24.04 (x86-64) | AMD EPYC 7763 (aes, pclmulqdq, vaes) | x86ni + pclmul: 2,373 to 2,496 MiB/s | 3.0.13: 3,333 to 3,509 MiB/s | about 1.4x |
| ubuntu-24.04-arm (aarch64) | not reported by /proc/cpuinfo | ct64 + ctmul64: 92.4 to 92.5 MiB/s | 3.0.13: 2,821 to 2,825 MiB/s | about 30x |
| macos-15 (arm64) | Apple M1 (virtual) | ct64 + ctmul64: 90.1 to 94.1 MiB/s | 3.6.4: 5,571 to 5,908 MiB/s | about 62x |

BearSSL's system seeder was rdrand on x86-64, getentropy on Linux arm64 and urandom on
macOS. webcpp's config should name the seeder rather than take BearSSL's first choice.

## 8. Built from sources, named by the user (2026-10-11)

For the owner's decision "2026-10-11: C libraries built from their own sources, and the backend
named in the user's Jamfile" (`~/.buzz/PLANS/WEBCPP_DESIGN.md`, last entry). Everything below was
built and run on 2026-10-11 on the machine of the Setup table. New probes, all under
`~/dev/webcpp/.superpowers/sdd/plan-reviews/c-backends/`:

| Probe | What it is |
| --- | --- |
| `arch/sizes.sh`, `arch/one.sh` | BearSSL file by file into an archive, against the unity `all_renamed.c`, same flags; tables in `arch/sizes-table.txt` |
| `arch/secp.sh` | libsecp256k1 as its CMake builds it (3 objects) against `probe/secp256k1/src.c`; `arch/secp-table.txt` |
| `jam/` | A scratch superproject: `Jamroot` (webcpp's wasi and emscripten settings; webcpp's own `tools/webcpp.jam` loaded unchanged), `third_party/Jamfile`, `crypto/` (header-only `/webcpp/crypto` sketch and the canary `.c` files), `app/<case>/Jamfile` (one user Jamfile per case), `sizes/` (the size check through b2), logs in `jam/out/` |
| `canary/canary.sh` | The link-time canary without b2, on ld64, wasm-ld and Emscripten; `canary/canary.log` |
| `msvc/` | The MSVC layer: clang in cl mode with lld-link, and real `link.exe` through Compiler Explorer (`ce*.c`, `ce*.out.json`) |
| `bitcoin-v31.1/` | Bitcoin Core tag v31.1 (commit `9be056a8a72b624dae9623b2f7bded92c2a21c91`, 2026-07-06), sparse checkout of `src/crypto/ctaes` only (tree `1b6c31139a71f80245c09597c343936a8e41d021`); ctaes is MIT, "Copyright (c) 2016 Pieter Wuille" |

No repository was modified.

### 8.1 Sizes: archive against unity

Same program as section 3 (`probe/bench/seal_bearssl.c`), same compiler flags on both sides
(`-O2` or `-Os`, `-ffunction-sections -fdata-sections`, `_DEFAULT_SOURCE` and
`BR_USE_GETENTROPY=1` on WASI and Emscripten), linked with `-Wl,-dead_strip` (macOS),
`--gc-sections --strip-all` (WASI) or emcc's defaults. Archive: the 294 files of `src/` compiled
one by one into `libbearssl.a`. Unity: `probe/bearssl/all_renamed.c`, rebuilt today. Bytes, raw /
`gzip -9n` (the `-n` matters: plain `gzip -9` stores the file name, which made the gzip column of
section 3 a few bytes larger and made two equal files look different).

| Target | Opt | Archive | Unity | Archive minus unity |
| --- | --- | --- | --- | --- |
| emscripten `.wasm` | -O2 | 30,072 / 13,789 | 32,508 / 14,000 | -2,436 / -211 |
| emscripten `.wasm` | -Os | 26,387 / 12,613 | 27,030 / 12,732 | -643 / -119 |
| wasip2 component, stripped | -O2 | 62,890 / 23,799 | 65,275 / 24,033 | -2,385 / -234 |
| wasip2 component, stripped | -Os | 59,887 / 22,872 | 60,570 / 23,015 | -683 / -143 |
| wasip3 component, stripped | -O2 | 70,875 / 26,495 | 73,262 / 26,716 | -2,387 / -221 |
| wasip3 component, stripped | -Os | 67,871 / 25,622 | 68,557 / 25,762 | -686 / -140 |
| native arm64 `__text` | -O2 / -Os | 14,188 / 11,428 | 14,320 / 11,808 | -132 / -380 |
| native arm64 file, stripped | -O2 / -Os | 51,648 / 35,120 | 51,472 / 35,024 | +176 / +96 (link-edit tables, not code) |

The Emscripten `.js` glue is byte-identical between the two once the `.wasm` file name it embeds
is the same. So the archive is never larger in code: on WebAssembly it is 0.6 to 2.4 KB smaller.
Built through b2 instead (`jam/sizes/`, `variant=release <optimization>space`, the archive being
`/third_party//bearssl` itself): native 36,920 against 36,776 (`__text` 11,428 against 11,808),
wasip2 59,887 against 60,570, wasip3 67,871 against 68,557 (equal to the table above), emscripten
35,867 against 36,567 (b2's emscripten release link is larger than plain `emcc -Os` for both).

libsecp256k1 (`probe/bench/secp_use.c`: key pair, BIP340 sign and verify, ECDH), its CMake build
(`secp256k1.c`, `precomputed_ecmult.c`, `precomputed_ecmult_gen.c`, every module on) in an archive,
against the unity `src.c`, raw / gzip -9n:

| Target | Opt | Default tables, archive | Default, unity | Small tables, archive | Small, unity |
| --- | --- | --- | --- | --- | --- |
| emscripten | -Os | 1,211,251 / 1,125,458 | 1,211,371 / 1,125,517 | 141,922 / 55,979 | 141,929 / 55,984 |
| emscripten | -O2 | 1,228,378 / 1,132,288 | 1,228,504 / 1,132,378 | 159,016 / 62,334 | 159,023 / 62,339 |
| wasip2 | -Os | 1,256,034 / 1,136,956 | 1,256,034 / 1,136,988 | 187,107 / 66,658 | 187,107 / 66,658 |
| wasip2 | -O2 | 1,280,653 / 1,143,971 | 1,280,653 / 1,144,015 | 211,675 / 74,079 | 211,675 / 74,079 |
| wasip3 | -Os | 1,263,835 / 1,139,627 | 1,263,835 / 1,139,667 | 194,907 / 69,264 | 194,907 / 69,268 |
| wasip3 | -O2 | 1,288,457 / 1,146,476 | 1,288,457 / 1,146,509 | 219,478 / 76,731 | 219,478 / 76,735 |
| native | -Os | 1,196,600 | 1,196,600 | 106,808 | 106,808 |
| native | -O2 | 1,195,352 | 1,195,352 | 122,072 | 122,072 |

libsecp256k1's `secp256k1.c` is already its own unity build, so the two are equal to within
120 bytes. (Section 4 calls the wasip2 default "not stripped"; it was stripped, the same 1,256,034.)

**A library named and not called costs nothing.** `probe/bench/baseline.c` (no cryptography)
linked with and without `libbearssl.a` or `libsecp256k1.a`: byte-identical on wasip2, wasip3 and
Emscripten (`cmp`), and natively byte-identical when both are linked under the same output name
(ld64's ad hoc signature embeds the output's name: 33,528 against 33,536 bytes under the names
`base-alone` and `base-archive`). Through b2: `exe unused : unused.cpp /third_party//bearssl ;`
and `exe unused : unused.cpp ;` (`jam/app/unused-named`, `jam/app/unused-alone`) are
byte-identical on all four targets.

**Correction: the POWER8 sources must be built on every target.** `aes_pwr8*.c` and
`ghash_pwr8.c` compile, off POWER, to stubs that return NULL (`br_aes_pwr8_ctr_get_vtable`,
`br_ghash_pwr8_get`), which BearSSL's own `ssl_engine_default_aes{cbc,ccm,gcm}.c` and the backend
call. An archive without them fails to link natively: `Undefined symbols: "_br_aes_pwr8_ctr_get_vtable",
"_br_ghash_pwr8_get"`. The rule is the opposite of "only on POWER": every `.c` of `src/`, on every
target, in the compiler's GNU dialect (no `-std`), which the POWER8 files need on POWER (section 1a).

### 8.2 The Jamfiles

`jam/third_party/Jamfile` (project `/third_party`) declares `bearssl` (`[ glob src/*.c src/*/*.c ]`,
294 files, no basename repeats, so b2's flat object directory is safe), `secp256k1` (the three CMake
files), `ctaes` (`ctaes.c`) and `openssl` (found, not built). Each webcpp-built library adds
webcpp's `crypto/src/canary_<name>.c` to the upstream sources. Project requirements: `<link>static`,
`<warnings>off` (upstream code is not held to webcpp's `-Werror`), and
`-ffunction-sections -fdata-sections` for clang, gcc and emscripten (`/Gy /Gw` for msvc). Usage
requirements: the public include directory, `WEBCPP_CRYPTO_HAS_<NAME>=1`, and the link-time section
GC (`-Wl,-dead_strip` on darwin, `-Wl,--gc-sections` on linux; wasm-ld and emcc do it by default).
b2 compiles each file as `clang++ -x c -O3 -w -ffunction-sections -fdata-sections ...` (no `-std`,
so the GNU dialect) and links the program with `-Wl,-dead_strip`. Two features, both propagated, so
that a program's request reaches the library: `webcpp-secp256k1-tables=default|small` and
`webcpp-crypto-openssl-link=shared|static`. Measured: `exe sign : sign.cpp /webcpp/crypto//crypto
/third_party//secp256k1 : <webcpp-secp256k1-tables>small ;` builds a second `libsecp256k1.a`
(296,096 bytes against 1,365,064) and the program is 122,152 bytes natively against 1,195,448.

Built and run with b2 on native (clang-darwin), clang-wasip2, clang-wasip3 and emscripten, each
printing its check:

| Case (`jam/app/<case>/Jamfile`) | The user's line | Result on all four targets |
| --- | --- | --- |
| bearssl | `exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ;` | `aes256_gcm ok backend=bearssl` (NIST GCM test case 14, seal and open) |
| sign | `exe sign : sign.cpp /webcpp/crypto//crypto /third_party//secp256k1 ;` | `schnorr ok` |
| sign-small | the same, `: <webcpp-secp256k1-tables>small ;` | `schnorr ok` |
| ctaes | `exe block : block.cpp /webcpp/crypto//crypto /third_party//ctaes ;` | `ctaes aes256 ok` (FIPS-197 C.3) |
| encrypt-and-sign | `exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl /third_party//secp256k1 ;` | `encrypt and sign ok` (two backends in one program, no duplicate symbols) |
| missing | `exe app : app.cpp /webcpp/crypto//crypto ;` | compile error, below |
| sign-missing | `exe sign : sign.cpp /webcpp/crypto//crypto ;` | compile error, below |
| openssl (native) | `exe app : app.cpp /webcpp/crypto//crypto /third_party//openssl ;` | `aes256_gcm ok backend=openssl`, 34,864 bytes, `libcrypto.3.dylib` |
| openssl-static (native) | the same, `: <webcpp-crypto-openssl-link>static ;`, with `-sOPENSSL_ROOT=/opt/homebrew/opt/openssl@3` | `aes256_gcm ok backend=openssl`, 3,623,296 bytes, no libcrypto dependency |
| both (native) | `... /third_party//openssl /third_party//bearssl ;` | `backend=openssl`: natively OpenSSL is the default when linked |
| select-bearssl (native) | the same, `: <define>WEBCPP_CRYPTO_DEFAULT_BACKEND_BEARSSL ;` | `backend=bearssl` |

### 8.3 The three layers of enforcement

**Compile time, the HAS macro (b2 users).** `crypto/include/webcpp/crypto/config.hpp` classifies
the target, refuses impossible combinations and picks the default AEAD backend
(`WEBCPP_CRYPTO_DEFAULT_BACKEND_<NAME>` when the program selects one; else BearSSL on WebAssembly,
OpenSSL natively when linked, else BearSSL). The check that the chosen backend is linked sits in the
backend's own header (`backends/<name>.hpp`, `secp256k1.hpp`), which the algorithms include, so that
a program using only libsecp256k1 needs no encryption backend. Exact texts, as clang prints them:

- No backend, WebAssembly (wasip2, wasip3, emscripten, `jam/out/missing-*.log`):
  `error: "webcpp::crypto: no backend is linked. On WebAssembly the backend is BearSSL: in the program's Jamfile, add /third_party//bearssl beside /webcpp/crypto//crypto, as in: exe app : app.cpp /webcpp/crypto//crypto /third_party//bearssl ; (without b2: define WEBCPP_CRYPTO_HAS_BEARSSL=1 and link BearSSL built with webcpp's src/canary_bearssl.c)"`
- No backend, native:
  `error: "webcpp::crypto: no backend is linked. In the program's Jamfile, add /third_party//openssl (the system's OpenSSL) or /third_party//bearssl (built from source) beside /webcpp/crypto//crypto, as in: exe app : app.cpp /webcpp/crypto//crypto /third_party//openssl ; (without b2: ...)"`
- A selected backend not linked: `"webcpp::crypto: the backend BearSSL is used and not linked. In the program's Jamfile, add /third_party//bearssl beside /webcpp/crypto//crypto. ..."`, the same for OpenSSL and ctaes.
- libsecp256k1: `"webcpp::crypto: <webcpp/crypto/secp256k1.hpp> needs libsecp256k1, which is not linked. In the program's Jamfile, add /third_party//secp256k1 beside /webcpp/crypto//crypto. ..."`
- OpenSSL on WebAssembly: b2 stops before compiling (`jam/out/openssl-emscripten.log`):
  `error: /third_party//openssl: OpenSSL is a native backend, and this build is for WebAssembly. In the program's Jamfile, name /third_party//bearssl in its place.`; without b2 `config.hpp` says the same.

clang and gcc do not stop at `#error`: the first error is the instruction, then one
`use of undeclared identifier 'webcpp'` per use follows (the header defines nothing after its
`#error`). MSVC stops there: `fatal error C1189: #error: "webcpp::crypto: ..."` (Compiler Explorer,
`x64 msvc v19.latest`). A b2 pitfall met on the way: two `exe` of one Jamfile built from the same
`.cpp`, differing only by a `/third_party` library, share one object file (defines are free features
and do not change its path), so b2 reused the object compiled with `WEBCPP_CRYPTO_HAS_BEARSSL` for
the program without it, which then failed at link instead. Each case of `jam/app/` therefore has a
directory of its own; webcpp's tests of these errors must do the same.

**Link time, the canary (everyone, and the only check without b2).** `detail/canary.h` (C and C++)
holds the pins (`7bea48e`, `6e2c8bc`, `9be056a`) and builds the name
`webcpp_crypto_<name>_<commit7>_<configuration>__add_third_party_<name>_to_your_link`.
`crypto/src/canary_<name>.c`, added to the library's sources by its Jamfile, defines it from the
library's own compile flags; the backend's header references it with a volatile read in every
function that calls the C library, so no linker can drop the reference. Configurations: BearSSL's
system seeder (`sysrng` natively; on WebAssembly `getentropy`, or `noseeder` when built without
`BR_USE_GETENTROPY`, which otherwise links and silently has no seeder), libsecp256k1's tables
(`w15c11x6`, `w2c2x5`), ctaes none (`default`). Cost: one byte and one load. Messages
(`canary/canary.log`), for BearSSL missing altogether, BearSSL built without the canary (its own
Makefile), BearSSL with the canary of another commit (`8f795e5`, BearSSL's previous one), BearSSL
for wasm without `BR_USE_GETENTROPY`, and libsecp256k1 with the small tables against a program
expecting the default:

| Linker | Message (the canary line; everything else is the same for each case) |
| --- | --- |
| ld64 (Apple clang 21, native) | `Undefined symbols for architecture arm64:` / `"_webcpp_crypto_bearssl_7bea48e_sysrng__add_third_party_bearssl_to_your_link", referenced from: webcpp::crypto::backends::bearssl::gcm::gcm(...) in app.o`; secp256k1: `"_webcpp_crypto_secp256k1_6e2c8bc_w15c11x6__add_third_party_secp256k1_to_your_link", referenced from: webcpp::crypto::secp256k1::schnorr_key::schnorr_key() in sign.o` |
| wasm-ld (wasi-sdk 34, wasip2) | `wasm-ld: error: .../app.o: undefined symbol: webcpp_crypto_bearssl_7bea48e_getentropy__add_third_party_bearssl_to_your_link`, then `error: failed to invoke LLD: exit status: 1` |
| Emscripten 6.0.11 (its wasm-ld) | `wasm-ld: error: .../app.o: undefined symbol: webcpp_crypto_bearssl_7bea48e_getentropy__add_third_party_bearssl_to_your_link`, then `em++: error: '<the whole wasm-ld command>' failed (returned 1)` |

With the library absent the canary is one line among BearSSL's own undefined `br_*` symbols (ld64
lists them alphabetically, the canary last; wasm-ld in reference order, the canary fifth); with a
library of another commit or configuration, or one built without the canary, it is the only error.
Linked right, the programs run on all three (`aes256_gcm ok backend=bearssl`).

**MSVC, `#pragma detect_mismatch`.** Each backend header and each `canary_<name>.c` carry
`#pragma detect_mismatch("webcpp_crypto_backend_<name>", "<name>-<commit7>-<configuration>")`
under `_MSC_VER`. The key must be per backend: one key for all would make a program using BearSSL
and libsecp256k1 fail with a false mismatch. MSVC was not available here; two substitutes:

- clang in cl mode (`--target=x86_64-pc-windows-msvc`) writes
  `/FAILIFMISMATCH:"webcpp_crypto_backend_bearssl=bearssl-7bea48e-sysrng"` into `.drectve`, and
  lld-link (wasi-sdk 34) prints `lld-link: error: /failifmismatch: mismatch detected for
  'webcpp_crypto_backend_bearssl':` / `>>> use.obj has value bearssl-7bea48e-sysrng` /
  `>>> canary-other.obj has value bearssl-8f795e5-sysrng`, and for a missing library
  `lld-link: error: undefined symbol: webcpp_crypto_bearssl_7bea48e_sysrng__add_third_party_bearssl_to_your_link`.
- Compiler Explorer offers MSVC with linking and execution (118 MSVC compilers, `supportsExecute`
  true). With `x64 msvc v19.latest`, one source file: `error LNK2001: unresolved external symbol
  webcpp_crypto_bearssl_7bea48e_sysrng__add_third_party_bearssl_to_your_link` then `fatal error
  LNK1120: 1 unresolved externals`; and `error LNK2038: mismatch detected for
  'webcpp_crypto_backend_bearssl': value 'bearssl-8f795e5-sysrng' doesn't match value
  'bearssl-7bea48e-sysrng' in output.s.obj` then `fatal error LNK1319: 1 mismatches detected`. The
  pragma's value is macro-expanded (built here with the stringizing macros of `canary.h`), and the
  canary name with a keyword as configuration (`default`) pastes fine.

What detect_mismatch adds, measured with lld-link: through a static library the canary's member is
loaded only when its name matches, so a library of another commit gives the unresolved canary
(LNK2001), not LNK2038. LNK2038 fires when the mismatching objects are both linked: two of the
program's own translation units compiled with different configurations (measured:
`use.obj has value bearssl-7bea48e-sysrng` / `use2.obj has value bearssl-7bea48e-getentropy`), or
a canary object linked directly. It is a second net, chiefly between a program's own files.

By reasoning only (no MSVC build of the libraries was run): BearSSL builds with MSVC (its
`conf/Win.mk`, `BR_MSC_2005` intrinsics) and links `advapi32` itself (`src/rand/sysrng.c:42`,
`#pragma comment(lib, "advapi32")`); libsecp256k1 static needs `SECP256K1_STATIC` in the program
(in the usage requirements above, as trystero's `build.jam` already does); `/Gy /Gw` place functions
and data in sections, and link.exe's `/OPT:REF` is on by default only without `/DEBUG`, so the
usage requirements should add `<toolset>msvc:<linkflags>/OPT:REF` for debug-symbol builds.

### 8.4 OpenSSL through webcpp.external

`jam/third_party/Jamfile` loads `~/dev/webcpp/tools/webcpp.jam` unchanged and declares
`webcpp.external crypto : OpenSSL : OPENSSL_ROOT : openssl : openssl/evp.h : native`, two
`openssl-crypto` alternatives by `webcpp-crypto-openssl-link` (`<file>.../lib/libcrypto.a` for
static, `<name>crypto <search>.../lib` for shared), and
`alias openssl : openssl-crypto : <conditional>@openssl-found : : <define>WEBCPP_CRYPTO_HAS_OPENSSL=1 ...`.
`openssl-found` stops the build on emscripten, wasip2 and wasip3 (message in 8.3) and otherwise
returns `webcpp.external-found crypto : OpenSSL`. Measured: shared from `.local/openssl` (the
default), static with `-sOPENSSL_ROOT=/opt/homebrew/opt/openssl@3` (table in 8.2). Not found
(a copy of `tools/` under a root without `.local/openssl`, `-sWEBCPP_TOOLS=/tmp/fakeroot/tools`):
`webcpp: crypto is left out: OpenSSL was not found: -sOPENSSL_ROOT=<dir> was not given, and
/tmp/fakeroot/.local/openssl/include holds no openssl/evp.h, nor does /usr/include or
/usr/local/include. Give -sOPENSSL_ROOT=<dir>, the directory of its include/ and lib/, or install
it in /tmp/fakeroot/.local/openssl, as libs/crypto/README.md says.`, b2 exits 0 and builds nothing;
with `webcpp-require-external=on` the build stops instead: `error: crypto needs OpenSSL, which was
not found, and the build requires it (webcpp-require-external=on): ...`; a `-sOPENSSL_ROOT` without
the header: `error: -sOPENSSL_ROOT=/tmp/no-openssl holds no include/openssl/evp.h: give the
directory of OpenSSL's include/ and lib/, or no -sOPENSSL_ROOT to look for OpenSSL in
.../.local/openssl, else on the system.` For a user's own program "left out, exit 0" is a trap
(the program silently does not exist); an owner decision is whether `/third_party//openssl` should
stop the build instead, as a user who names it has asked for it.

### 8.5 Drafts

- Guideline: `~/dev/webcpp/.superpowers/sdd/plan-reviews/c-backends/guideline-draft.md`.
- User section: `~/dev/webcpp/.superpowers/sdd/plan-reviews/c-backends/using-crypto-draft.md`;
  every snippet in it is one of `jam/app/<case>/Jamfile`, built and run above.
