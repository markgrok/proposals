// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Every number json_schema compares, as its exact decimal: a schema's operand, and an instance's
 integer or double; and what the loader keeps of a schema's double.

 A double is the decimal its shortest round-trip text denotes, never its binary value and never
 a value rounded to `Number`, on the instance's side and the schema's alike: the double `1.1` is
 the decimal 1.1, and the double 2^64 is 18446744073709552000.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_NUMBERS_DECOMPOSE_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_NUMBERS_DECOMPOSE_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/numbers/arithmetic.hpp>
#include <webcpp/json_schema/ir/number.hpp>
#include <webcpp/json_schema/numbers/number.hpp>

#include <boost/json/value.hpp>

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

namespace webcpp::json_schema::detail {

/** The decomposition of `i`, exact for `INT64_MIN` too. */
inline decomposed decompose_int64(std::int64_t i) noexcept {
    const bool negative = i < 0;
    const std::uint64_t m =
        negative ? 0 - static_cast<std::uint64_t>(i) : static_cast<std::uint64_t>(i);
    return {.negative = negative, .coefficient = {.high = 0, .low = m}, .exponent = 0};
}

/** The decomposition of a schema's number: an integer exactly, a `Number` by its traits. */
template <class Number>
decomposed decompose(const ir::number<Number>& n) noexcept {
    if (const auto* i = std::get_if<0>(&n)) {
        return decompose_int64(*i);
    }
    if (const auto* u = std::get_if<1>(&n)) {
        return {.negative = false, .coefficient = {.high = 0, .low = *u}, .exponent = 0};
    }
    const auto* number = std::get_if<2>(&n);
    return number == nullptr ? decomposed{} : number_traits<Number>::decompose(*number);
}

/**
 The shortest text that reads back as the finite `d`, in scientific form: "1.8446744073709552e+19".

 Tip: the form is named: the default one writes a large integral double in fixed notation, with
 every digit of its binary value: 2^60 as 1152921504606846976, not 1.152921504606847e+18.
*/
inline std::string_view shortest_text(double d, std::array<char, 32>& buffer) noexcept {
    const auto written = std::to_chars(buffer.data(), buffer.data() + buffer.size(), d,
                                       std::chars_format::scientific);
    if (written.ec != std::errc{}) {
        return {};
    }
    return {buffer.data(), static_cast<std::size_t>(written.ptr - buffer.data())};
}

/**
 The decimal the shortest round-trip text of `d` denotes, exactly, 17 significant digits at most;
 nothing for an infinity or a NaN.
*/
inline std::optional<decomposed> decompose_double(double d) noexcept {
    if (!std::isfinite(d)) {
        return std::nullopt;
    }
    std::array<char, 32> buffer{};
    const std::string_view text = shortest_text(d, buffer);
    const std::size_t e = text.find('e');
    if (e == std::string_view::npos) {
        return std::nullopt;
    }
    const bool negative = text.front() == '-';
    std::string_view mantissa = text;
    mantissa.remove_suffix(text.size() - e);
    mantissa.remove_prefix(negative ? 1 : 0);
    u128 coefficient = 0U;
    std::int64_t exponent = 0;
    bool after_point = false;
    for (const char c : mantissa) {
        if (c == '.') {
            after_point = true;
            continue;
        }
        coefficient = coefficient * 10U + static_cast<unsigned>(c - '0');
        exponent -= after_point ? 1 : 0;
    }
    std::string_view power = text;
    power.remove_prefix(e + 1);
    power.remove_prefix(power.front() == '+' ? 1 : 0);
    int written_exponent = 0;
    const auto read = std::from_chars(power.data(), power.data() + power.size(), written_exponent);
    if (read.ec != std::errc{}) {
        return std::nullopt;
    }
    return decomposed{
        .negative = negative,
        .coefficient = from_u128(coefficient),
        .exponent = static_cast<std::int32_t>(exponent + written_exponent),
    };
}

/**
 The decomposition of an instance's number: an integer exactly, a double as above; nothing for
 any other value.
*/
inline std::optional<decomposed> decompose_value(const boost::json::value& v) noexcept {
    if (const auto* i = v.if_int64()) {
        return decompose_int64(*i);
    }
    if (const auto* u = v.if_uint64()) {
        return decomposed{.negative = false, .coefficient = {.high = 0, .low = *u}, .exponent = 0};
    }
    if (const auto* d = v.if_double()) {
        return decompose_double(*d);
    }
    return std::nullopt;
}

/**
 The integer `value` is, when it is one that `std::int64_t` or `std::uint64_t` holds.

 Tip: the integer is the decimal's, which for a double at or past 2^53 is its shortest text's,
 not its binary value's, so a schema's double and the same double in an instance stay equal.
*/
template <class Number>
std::optional<ir::number<Number>> integer_of(const decomposed& value) noexcept {
    const magnitude m = normalize(value);
    if (m.exponent < 0 || digits(m.coefficient) + m.exponent > 20) {
        return std::nullopt;
    }
    const auto whole = scale(m.coefficient, m.exponent);
    if (!whole) {
        return std::nullopt;
    }
    const u128 two_to_63 = u128{1U} << 63;
    if (value.negative && *whole <= two_to_63) {
        const auto twos_complement = 0 - static_cast<std::uint64_t>(*whole);
        return ir::number<Number>{
            std::in_place_index<0>,
            static_cast<std::int64_t>(twos_complement),
        };
    }
    if (!value.negative && *whole < two_to_63) {
        return ir::number<Number>{std::in_place_index<0>, static_cast<std::int64_t>(*whole)};
    }
    if (!value.negative && *whole <= u128{std::numeric_limits<std::uint64_t>::max()}) {
        return ir::number<Number>{std::in_place_index<1>, static_cast<std::uint64_t>(*whole)};
    }
    return std::nullopt;
}

/**
 What the loader keeps of a schema's double, by invariant 7: the decimal of its shortest text,
 as an integer when it is one that fits `std::int64_t` or `std::uint64_t`, else as a `Number`
 read from that text; nothing for an infinity, a NaN, or a value `Number` cannot hold.
*/
template <class Number>
std::optional<ir::number<Number>> schema_number_of_double(double d) noexcept {
    const auto value = decompose_double(d);
    if (!value) {
        return std::nullopt;
    }
    if (auto integer = integer_of<Number>(*value)) {
        return integer;
    }
    std::array<char, 32> buffer{};
    const auto n = number_traits<Number>::from_chars(shortest_text(d, buffer));
    if (!n) {
        return std::nullopt;
    }
    return ir::number<Number>{std::in_place_index<2>, n.unsafe_value()};
}

/**
 Whether the instance's `v` is an integer: an `int64_t` or a `uint64_t`, or, when
 `integer_by_value`, a double without a fraction (every dialect but draft 4).
*/
inline bool is_integer_value(const boost::json::value& v, bool integer_by_value) noexcept {
    if (v.is_int64() || v.is_uint64()) {
        return true;
    }
    if (const auto* d = v.if_double()) {
        return integer_by_value && std::isfinite(*d) && std::trunc(*d) == *d;
    }
    return false;
}

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_NUMBERS_DECOMPOSE_HPP
