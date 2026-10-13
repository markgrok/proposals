// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The 128-bit arithmetic that comparisons and `multipleOf` run on: decimal digits, powers of ten,
 and products modulo a number, none of which overflows.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_NUMBERS_ARITHMETIC_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_NUMBERS_ARITHMETIC_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/numbers/number.hpp>

#include <boost/decimal/uint128_t.hpp>

#include <cstdint>
#include <limits>
#include <optional>

namespace webcpp::json_schema::detail {

/**
 An unsigned integer of 128 bits, Boost.Int128's, through Boost.Decimal.

 Tip: Boost 1.92's `uint128_t & n`, for an integer `n` of another type, keeps the upper half
 whole, so this code never mixes types in a bitwise operation.
*/
using u128 = boost::decimal::uint128_t;

/** The integer the two halves of `v` make. */
inline u128 to_u128(const uint128& v) noexcept {
    return (static_cast<u128>(v.high) << 64) | static_cast<u128>(v.low);
}

/** The two halves of `v`. */
inline uint128 from_u128(u128 v) noexcept {
    return {.high = static_cast<std::uint64_t>(v >> 64), .low = static_cast<std::uint64_t>(v)};
}

/** How many decimal digits `c` has: 1 for zero, 39 at most. */
inline std::int64_t digits(u128 c) noexcept {
    std::int64_t count = 0;
    do {
        ++count;
        c /= 10U;
    } while (c != 0U);
    return count;
}

/** `c * 10^n` for `n >= 0`, or nothing when it exceeds 128 bits. */
inline std::optional<u128> scale(u128 c, std::int64_t n) noexcept {
    if (c == 0U) {
        return c;
    }
    const u128 limit = std::numeric_limits<u128>::max() / 10U;
    for (std::int64_t i = 0; i < n; ++i) {
        if (c > limit) {
            return std::nullopt;
        }
        c *= 10U;
    }
    return c;
}

/** `(a + b) mod m` for `a` and `b` below `m`, which never overflows, whatever `m`. */
inline u128 add_mod(u128 a, u128 b, u128 m) noexcept {
    return a >= m - b ? a - (m - b) : a + b;
}

/** `(a * b) mod m` for `m > 0`, by doubling, which never overflows, whatever `m`. */
inline u128 mulmod(u128 a, u128 b, u128 m) noexcept {
    u128 result = 0U;
    a %= m;
    while (b != 0U) {
        if ((static_cast<std::uint64_t>(b) & 1U) != 0U) {
            result = add_mod(result, a, m);
        }
        a = add_mod(a, a, m);
        b >>= 1U;
    }
    return result;
}

/** `10^n mod m` for `m > 0`, by squaring. */
inline u128 pow10_mod(std::uint64_t n, u128 m) noexcept {
    u128 result = u128{1U} % m;
    u128 base = u128{10U} % m;
    for (; n != 0; n >>= 1U) {
        if ((n & 1U) != 0U) {
            result = mulmod(result, base, m);
        }
        base = mulmod(base, base, m);
    }
    return result;
}

/** `|value| = coefficient * 10^exponent`, no trailing zero in the coefficient; zero is {0, 0}. */
struct magnitude {
    /** The coefficient, without trailing zeros. */
    u128 coefficient{};

    /** The power of ten, wide enough that no sum of two of them overflows. */
    std::int64_t exponent = 0;
};

/** The magnitude of `d`, its trailing zeros moved into the exponent. */
inline magnitude normalize(const decomposed& d) noexcept {
    u128 c = to_u128(d.coefficient);
    std::int64_t e = d.exponent;
    if (c == 0U) {
        return {.coefficient = 0U, .exponent = 0};
    }
    while (c % 10U == 0U) {
        c /= 10U;
        ++e;
    }
    return {.coefficient = c, .exponent = e};
}

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_NUMBERS_ARITHMETIC_HPP
