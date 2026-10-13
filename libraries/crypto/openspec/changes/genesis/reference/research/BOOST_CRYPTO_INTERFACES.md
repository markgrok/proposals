---
title: "What Boost has for cryptography, and the shape of its interfaces (Boost 1.92)"
tags: [webcpp, crypto, boost, research]
status: active
created: 2026-10-10
---

# Boost's cryptography, read from the installed headers (/opt/homebrew/include/boost, 1.92)

## Boost.Hash2 (public)

- **Algorithms:**
  - `sha1_160`, `sha2_224` / `sha2_256` / `sha2_384` / `sha2_512`, SHA-3, `md5_128`, `ripemd_*`, BLAKE2;
  - `siphash`, `xxhash`, `xxh3`, `fnv1a`.
- **One shape for every algorithm** (`hash2/sha2.hpp:508-580`):
  - `using result_type = digest<N>;`
  - `static constexpr std::size_t block_size;`
  - constructors: default; `explicit (std::uint64_t seed)`; `(unsigned char const* p, std::size_t n)`, a byte seed or key;
  - `update(void const* p, std::size_t n)`, then `result()`.
  - Constexpr where it can be. No allocation, no exception.
- **`template<class H> class hmac`** (`hash2/hmac.hpp:38-140`):
  - it is itself a hash algorithm, with the same members;
  - the key is given through the byte-seed constructor;
  - modes compose as templates over an algorithm.
- **`template<std::size_t N> class digest`** (`hash2/digest.hpp`):
  - an array type with `data()`, `size()` and `operator[]`;
  - `to_chars`, `to_string` and `operator<<` (hex).
  - **Its `operator==` is `memcmp`, not constant time** (`hash2/detail/memcmp.hpp:21-23`). A MAC or tag must never be checked with it.

## Elsewhere in Boost

- Boost.UUID keeps a private `detail::chacha20_12` (`uuid/detail/chacha20.hpp`), seeded from `detail::random_device`, which is `std::random_device`, except on MinGW (`rand_s`).
- Boost.Random's `random_device` is a compiled library: a UniformRandomBitGenerator with `min`, `max` and `operator()`, which throws on failure.
- Boost.Beast keeps a private `detail::sha1` and `detail::base64`.
- No public cipher, AEAD, CSPRNG or constant-time comparison exists in Boost.

## The C++ Alliance (not part of Boost)

The C++ Alliance's crypt is not part of Boost. It has hashers, HMAC and DRBGs, and AES modes without GCM, built on a table S-box (see AES_GCM_WITHOUT_WEBCRYPTO.md).
