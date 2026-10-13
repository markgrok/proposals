---
title: "AES-GCM without WebCrypto: what C++, Boost and the C++ Alliance provide"
tags: [webcpp, crypto, aes-gcm, research]
status: active
created: 2026-10-10
---

# AES-GCM without WebCrypto

Question (owner, 2026-10-10): can webcpp::crypto do AES-GCM in C++ on every target, so the
browser build no longer calls WebCrypto?

## Findings

- **std:** no cryptography.
- **Boost:** no cipher. Boost.Hash2 has hashes and HMAC only.
- **C++ Alliance crypt** (github.com/cppalliance/crypt, BSL-1.0, checked through the GitHub
  API on 2026-10-10):
  - `include/boost/crypt/aes/` has AES-128/192/256 with the modes ECB, CBC, OFB, CTR,
    CFB8/64/128 (`aes/detail/cipher_mode.hpp`). No GCM and no GHASH.
  - The S-box is a 256-byte lookup table indexed by secret bytes
    (`aes/detail/cipher.hpp`, `sbox[temp[0]]`). That is not constant-time: its timing
    depends on the cache, and so on the key.
  - The rewrite `crypt2` has reached ECB only.
  - The last commit was on 2025-02-10.
  - It has hash and HMAC DRBGs (`drbg/`), which still need a seed from the system.
- **WebAssembly** has no AES instruction, so a constant-time AES in wasm must be bitsliced.
- **Randomness:** the browser's only entropy source is `crypto.getRandomValues`.
  Emscripten's `getentropy()` calls it synchronously, so C++ reaches it through libc
  without JS of its own. WASI has `random_get`.
- **Ported reference:** BearSSL (MIT) has constant-time, portable, bitsliced
  `aes_ct64` and `ghash_ctmul64`, and its GCM is built from them (`src/symcipher/`,
  `src/hash/ghash_ctmul64.c`).
  - Not yet checked: its current state and its license text at a pinned commit.

## Conclusion

AES-GCM can be done in C++ on every target. It cannot be taken ready-made from Boost, std
or the C++ Alliance: crypt lacks GCM and its AES is not constant-time.

## Who uses BearSSL, and what Bitcoin Core uses (checked 2026-10-10 through the GitHub API)

- **BearSSL in other projects:**
  - FreeBSD carries it in `contrib/bearssl`.
  - The ESP8266 Arduino core builds its TLS on it (`libraries/ESP8266WiFi/src/BearSSLHelpers.*`).
  - BoringSSL's software AES, `crypto/fipsmodule/aes/aes_nohw.cc.inc`, cites BearSSL's constant-time technique (bearssl.org/constanttime.html#aes).
  - Releases are rare.
- **Bitcoin Core** uses no outside cryptography library for symmetric crypto: OpenSSL left its build years ago.
  - `src/crypto/` holds its own SHA-1, SHA-256 (with SSE4, AVX2 and SHA-NI paths), SHA-512, SHA3, RIPEMD-160, HMAC, HKDF, SipHash, MuHash, ChaCha20, Poly1305 and ChaCha20-Poly1305 (BIP324's transport).
  - AES is `src/crypto/ctaes/`, Pieter Wuille's ctaes: pure C, bitsliced, no tables or data-dependent branches, about 4 KB of code at -Os, about 15 MB/s. It is wrapped in C++ by `src/crypto/aes.h` (AES256Encrypt, AES256CBCEncrypt) for the wallet's encryption.
  - Bitcoin Core has no GCM and no GHASH; its AEAD is ChaCha20-Poly1305.
  - Signatures are libsecp256k1, already trystero's dependency.
