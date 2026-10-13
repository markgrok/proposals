// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The exact order of two numbers, whatever their kinds: an instance's integer or double against a
 schema's integer or Number.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_NUMBERS_COMPARE_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_NUMBERS_COMPARE_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/numbers/arithmetic.hpp>
#include <webcpp/json_schema/numbers/number.hpp>

#include <compare>
#include <cstdint>

namespace webcpp::json_schema::detail {

/** The order of two 128-bit integers. */
inline std::strong_ordering order(u128 a, u128 b) noexcept {
    if (a < b) {
        return std::strong_ordering::less;
    }
    return a == b ? std::strong_ordering::equal : std::strong_ordering::greater;
}

/**
 The order of two magnitudes: by their orders of magnitude, then by their coefficients at one
 exponent.

 Tip: of two numbers of one order of magnitude, the scaled coefficient has the other's digit
 count, which fits 128 bits for every Number's coefficient; one that does not fit exceeds the
 other coefficient, which does.
*/
inline std::strong_ordering compare_magnitudes(const magnitude& x, const magnitude& y) noexcept {
    if (x.coefficient == 0U || y.coefficient == 0U) {
        return order(x.coefficient == 0U ? 0U : 1U, y.coefficient == 0U ? 0U : 1U);
    }
    const std::int64_t x_order = digits(x.coefficient) + x.exponent;
    const std::int64_t y_order = digits(y.coefficient) + y.exponent;
    if (x_order != y_order) {
        return x_order <=> y_order;
    }
    if (x.exponent >= y.exponent) {
        const auto scaled = scale(x.coefficient, x.exponent - y.exponent);
        return scaled ? order(*scaled, y.coefficient) : std::strong_ordering::greater;
    }
    const auto scaled = scale(y.coefficient, y.exponent - x.exponent);
    return scaled ? order(x.coefficient, *scaled) : std::strong_ordering::less;
}

/** The exact order of `a` and `b`; a zero of either sign is zero. */
inline std::strong_ordering compare(const decomposed& a, const decomposed& b) noexcept {
    const magnitude x = normalize(a);
    const magnitude y = normalize(b);
    const bool x_negative = a.negative && x.coefficient != 0U;
    const bool y_negative = b.negative && y.coefficient != 0U;
    if (x_negative != y_negative) {
        return x_negative ? std::strong_ordering::less : std::strong_ordering::greater;
    }
    const std::strong_ordering m = compare_magnitudes(x, y);
    return x_negative ? 0 <=> m : m;
}

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_NUMBERS_COMPARE_HPP
