// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 `multipleOf`, exactly, on decimal coefficients and exponents: never `fmod`, never a rounded
 quotient.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_NUMBERS_MULTIPLE_OF_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_NUMBERS_MULTIPLE_OF_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/numbers/arithmetic.hpp>
#include <webcpp/json_schema/numbers/number.hpp>

#include <cstdint>

namespace webcpp::json_schema::detail {

/**
 Whether `x` is an integer times `y`, for `y > 0`; signs are ignored.

 With `x = cx * 10^ex` and `y = cy * 10^ey`, each coefficient without trailing zeros: when `ex >=
 ey`, `x` is a multiple iff `cx * 10^(ex - ey) mod cy` is 0, which modular arithmetic gives in 128
 bits whatever the shift; when `ex < ey`, never, since `cy * 10^(ey - ex)` would have to divide
 `cx`, which 10 does not.
*/
inline bool is_multiple_of(const decomposed& x, const decomposed& y) noexcept {
    const magnitude a = normalize(x);
    const magnitude b = normalize(y);
    if (a.coefficient == 0U) {
        return true;
    }
    if (b.coefficient == 0U) {
        return false;  // y > 0 by [ir-7]; a zero here is no multiple of anything
    }
    if (a.exponent < b.exponent) {
        return false;
    }
    const u128 rest = a.coefficient % b.coefficient;
    const auto shift = static_cast<std::uint64_t>(a.exponent - b.exponent);
    return mulmod(rest, pow10_mod(shift, b.coefficient), b.coefficient) == 0U;
}

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_NUMBERS_MULTIPLE_OF_HPP
