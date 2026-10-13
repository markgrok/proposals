// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 Boost.Decimal's `decimal64_t`, the default number of a schema, and `decimal128_t`, the larger
 one, as models of `number`.
*/
#ifndef WEBCPP_JSON_SCHEMA_NUMBERS_DECIMAL_HPP
#define WEBCPP_JSON_SCHEMA_NUMBERS_DECIMAL_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/detail/numbers/arithmetic.hpp>
#include <webcpp/json_schema/errors.hpp>
#include <webcpp/json_schema/numbers/number.hpp>

#include <boost/decimal/charconv.hpp>
#include <boost/decimal/cmath.hpp>
#include <boost/decimal/decimal128_t.hpp>
#include <boost/decimal/decimal64_t.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/result.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <system_error>

namespace webcpp::json_schema {

namespace detail {

/** The position past the decimal digits of `text` from `at`. */
constexpr std::size_t skip_digits(std::string_view text, std::size_t at) noexcept {
    while (at < text.size() && text[at] >= '0' && text[at] <= '9') {
        ++at;
    }
    return at;
}

/** Whether `text` is a JSON number, as RFC 8259 writes one: no sign but `-`, no leading zero. */
constexpr bool is_json_number(std::string_view text) noexcept {
    std::size_t at = text.starts_with('-') ? 1 : 0;
    if (at == text.size() || text[at] < '0' || text[at] > '9') {
        return false;
    }
    at = text[at] == '0' ? at + 1 : skip_digits(text, at);
    if (at < text.size() && text[at] == '.') {
        const std::size_t fraction = skip_digits(text, at + 1);
        if (fraction == at + 1) {
            return false;
        }
        at = fraction;
    }
    if (at < text.size() && (text[at] == 'e' || text[at] == 'E')) {
        ++at;
        at += at < text.size() && (text[at] == '+' || text[at] == '-') ? 1 : 0;
        const std::size_t exponent = skip_digits(text, at);
        if (exponent == at) {
            return false;
        }
        at = exponent;
    }
    return at == text.size();
}

/**
 The digits of a JSON number's mantissa, its integer part then its fraction, read as one sequence
 without the point.
*/
struct mantissa_digits {
    /** The digits before the point. */
    std::string_view integer;

    /** The digits after the point, if any. */
    std::string_view fraction;

    /** How many digits there are. */
    [[nodiscard]] constexpr std::size_t size() const noexcept {
        return integer.size() + fraction.size();
    }

    /** The value of the digit at `k`. */
    [[nodiscard]] constexpr unsigned at(std::size_t k) const noexcept {
        const char c = k < integer.size() ? integer[k] : fraction[k - integer.size()];
        return static_cast<unsigned>(c - '0');
    }
};

/**
 A JSON number as its significant digits, from its first non-zero digit to its last, and the
 power of ten of the last.
*/
struct significant_digits {
    /** Whether the number has a minus sign, a zero included. */
    bool negative = false;

    /** The mantissa's digits. */
    mantissa_digits digits;

    /** Where the first non-zero digit is in `digits`. */
    std::size_t first = 0;

    /** How many digits run from the first non-zero one to the last; 0 for a zero. */
    std::int64_t count = 0;

    /** The power of ten of the last non-zero digit. */
    std::int64_t exponent = 0;
};

/**
 The magnitude past which an exponent is held as that magnitude: no number of either type is that
 large or small, and no sum the reading makes with it overflows 64 bits.
*/
inline constexpr std::int64_t exponent_limit = 1'000'000'000'000'000;

/** The exponent a JSON number writes after its `e`, held at @ref exponent_limit at most. */
constexpr std::int64_t saturated_exponent(std::string_view power) noexcept {
    const bool negative = power.starts_with('-');
    power.remove_prefix(power.starts_with('-') || power.starts_with('+') ? 1 : 0);
    std::int64_t value = 0;
    for (const char c : power) {
        value = std::min<std::int64_t>((value * 10) + (c - '0'), exponent_limit);
    }
    return negative ? -value : value;
}

/** The significant digits of `text`, a JSON number, read in one pass and without allocating. */
constexpr significant_digits significant_digits_of(std::string_view text) noexcept {
    significant_digits n;
    n.negative = text.starts_with('-');
    text.remove_prefix(n.negative ? 1 : 0);
    const std::size_t e = text.find_first_of("eE");
    std::string_view mantissa = text;
    std::string_view power;
    if (e != std::string_view::npos) {
        mantissa.remove_suffix(text.size() - e);
        power = text;
        power.remove_prefix(e + 1);
    }
    const std::size_t point = mantissa.find('.');
    n.digits.integer = mantissa;
    if (point != std::string_view::npos) {
        n.digits.integer.remove_suffix(mantissa.size() - point);
        n.digits.fraction = mantissa;
        n.digits.fraction.remove_prefix(point + 1);
    }
    std::size_t first = 0;
    while (first < n.digits.size() && n.digits.at(first) == 0) {
        ++first;
    }
    if (first == n.digits.size()) {
        return n;
    }
    std::size_t last = n.digits.size() - 1;
    while (n.digits.at(last) == 0) {
        --last;
    }
    const auto trailing_zeros = static_cast<std::int64_t>(n.digits.size() - 1 - last);
    n.first = first;
    n.count = static_cast<std::int64_t>(last - first) + 1;
    n.exponent = saturated_exponent(power) - static_cast<std::int64_t>(n.digits.fraction.size()) +
                 trailing_zeros;
    return n;
}

/** A decimal as an integer coefficient and a power of ten. */
struct rounded_decimal {
    /** The coefficient; 0 for a number that rounds to zero. */
    u128 coefficient{};

    /** The power of ten. */
    std::int64_t exponent = 0;
};

/** The first `count` significant digits of `n`, at most 38, as an integer. */
constexpr u128 coefficient_of(const significant_digits& n, std::int64_t count) noexcept {
    u128 c = 0U;
    for (std::int64_t k = 0; k < count; ++k) {
        c = (c * 10U) + n.digits.at(n.first + static_cast<std::size_t>(k));
    }
    return c;
}

/**
 `n` rounded to `precision` significant digits and to an exponent of at least `tiny`, ties to
 even, as IEEE 754 rounds a decimal: correctly, whatever the number of digits. A carry leaves
 `precision + 1` digits, a power of ten, which is the same number.

 Tip: the digit after the kept ones and whether any non-zero digit follows it decide the rounding;
 since the last significant digit is not zero, one follows exactly when the kept digits and that
 one are not all of them.
*/
constexpr rounded_decimal round_to(const significant_digits& n, std::int64_t precision,
                                   std::int64_t tiny) noexcept {
    const std::int64_t last = std::max(n.exponent + n.count - precision, tiny);
    if (last <= n.exponent) {
        return {.coefficient = coefficient_of(n, n.count), .exponent = n.exponent};
    }
    const std::int64_t kept = n.count - (last - n.exponent);
    if (kept < 0) {
        return {.coefficient = 0U, .exponent = last};
    }
    u128 c = coefficient_of(n, kept);
    const unsigned next = n.digits.at(n.first + static_cast<std::size_t>(kept));
    const bool sticky = n.count > kept + 1;
    const bool odd = (static_cast<std::uint64_t>(c) & 1U) != 0U;
    if (next > 5 || (next == 5 && (sticky || odd))) {
        c += 1U;
    }
    return {.coefficient = c, .exponent = last};
}

/** Writes `[-]coefficient e exponent` into `buffer`, which holds the longest, and returns it. */
inline std::string_view write_decimal(bool negative, const rounded_decimal& r,
                                      std::array<char, 64>& buffer) noexcept {
    std::array<char, 40> reversed{};
    std::size_t length = 0;
    u128 c = r.coefficient;
    do {
        reversed[length++] = static_cast<char>('0' + static_cast<unsigned>(c % 10U));
        c /= 10U;
    } while (c != 0U);
    std::size_t at = 0;
    if (negative) {
        buffer[at++] = '-';
    }
    while (length != 0) {
        buffer[at++] = reversed[--length];
    }
    buffer[at++] = 'e';
    const auto written =
        std::to_chars(buffer.data() + at, buffer.data() + buffer.size(), r.exponent);
    return {buffer.data(), static_cast<std::size_t>(written.ptr - buffer.data())};
}

/**
 The decimal type `T` of the JSON number `text`, rounded correctly to `T`'s digits, ties to even;
 `errc::invalid_number` for a number other than zero that rounds to zero or past `T`'s largest,
 and `boost::system::errc::invalid_argument` for a text that is no JSON number.

 Tip: Boost.Decimal 1.92 reads `1e`, `.5` and `5.`, which JSON does not; reads only the first 19
 digits of a `decimal64_t` text (37 of a `decimal128_t` one), so a tie they end rounds to even
 whatever follows; refuses `0e-100000`; and gives `1e-399` as zero. So the text is rounded here,
 and Boost.Decimal reads the result: `T`'s digits, or one more when rounding carries into a power
 of ten, which it holds exactly.
*/
template <class T>
boost::system::result<T> decimal_from_chars(std::string_view text) noexcept {
    if (!is_json_number(text)) {
        return boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
    }
    using limits = std::numeric_limits<T>;
    const significant_digits n = significant_digits_of(text);
    const rounded_decimal r =
        n.count == 0 ? rounded_decimal{}
                     : round_to(n, limits::digits, limits::min_exponent10 - (limits::digits - 1));
    if (n.count != 0 &&
        (r.coefficient == 0U || r.exponent + digits(r.coefficient) - 1 > limits::max_exponent10)) {
        return make_error_code(errc::invalid_number);
    }
    std::array<char, 64> buffer{};
    const std::string_view canonical = write_decimal(n.negative, r, buffer);
    T value{};
    const auto parsed =
        boost::decimal::from_chars(canonical.data(), canonical.data() + canonical.size(), value,
                                   boost::decimal::chars_format::scientific);
    if (parsed.ec != std::errc{} || !boost::decimal::isfinite(value)) {
        return make_error_code(errc::invalid_number);
    }
    return value;
}

/** The exact decomposition of the decimal `n`, its significand split into two halves. */
template <class T>
decomposed decimal_decompose(T n) noexcept {
    int exponent = 0;
    const auto significand = boost::decimal::frexp10(n, &exponent);
    return {
        .negative = boost::decimal::signbit(n),
        .coefficient = from_u128(static_cast<u128>(significand)),
        .exponent = static_cast<std::int32_t>(exponent),
    };
}

/** Whether the decimal `n` is an integer. */
template <class T>
bool decimal_is_integer(T n) noexcept {
    return boost::decimal::isfinite(n) && boost::decimal::trunc(n) == n;
}

/** Writes the decimal `n` into `out`, its shortest text; how many characters, or 0. */
template <class T>
std::size_t decimal_to_chars(std::span<char> out, T n) noexcept {
    const auto written = boost::decimal::to_chars(out.data(), out.data() + out.size(), n);
    return written.ec == std::errc{} ? static_cast<std::size_t>(written.ptr - out.data()) : 0;
}

}  // namespace detail

/**
 `boost::decimal::decimal64_t` as a schema's number: 16 significant digits, exponents from -398
 to 369, which every double's magnitude fits.

 A literal is rounded correctly to 16 significant digits, ties to even, whatever its length:
 `0.30000000000000004` is `0.3` here, `1.00000000000000050001` is `1.000000000000001`; below
 1e-383 it keeps the digits down to 1e-398, and so `5e-399`, which rounds to zero, is refused.

 @see "Numbers", in the guide.
*/
template <>
struct number_traits<boost::decimal::decimal64_t> {
    /** The number type. */
    using type = boost::decimal::decimal64_t;

    /**
     Reads a JSON number.

     @param text The number's text, as JSON writes it: `-1.5e-3`.
     @return The number, rounded correctly to 16 significant digits and to an exponent of at
     least -398, ties to even; @ref errc::invalid_number for a number other than zero that rounds
     to zero or past 9.999999999999999e384; `boost::system::errc::invalid_argument` for a text
     that is no JSON number.
    */
    static boost::system::result<type> from_chars(std::string_view text) noexcept {
        return detail::decimal_from_chars<type>(text);
    }

    /**
     The number's exact decomposition.

     @param n The number.
     @return Its sign, its 16-digit coefficient and its exponent.
    */
    static decomposed decompose(type n) noexcept { return detail::decimal_decompose(n); }

    /**
     Whether the number is an integer.

     @param n The number.
     @return `true` when `n` is finite and has no fraction.
    */
    static bool is_integer(type n) noexcept { return detail::decimal_is_integer(n); }

    /**
     Writes the number's shortest text.

     @param out Where to write it.
     @param n The number.
     @return How many characters were written, or 0 when they do not fit.
    */
    static std::size_t to_chars(std::span<char> out, type n) noexcept {
        return detail::decimal_to_chars(out, n);
    }
};

/**
 `boost::decimal::decimal128_t` as a schema's number: 34 significant digits, exponents from -6176
 to 6111, which holds every double's shortest text exactly.

 A literal is rounded correctly to 34 significant digits, ties to even, whatever its length.

 @see "Numbers", in the guide.
*/
template <>
struct number_traits<boost::decimal::decimal128_t> {
    /** The number type. */
    using type = boost::decimal::decimal128_t;

    /**
     Reads a JSON number.

     @param text The number's text, as JSON writes it: `-1.5e-3`.
     @return The number, rounded correctly to 34 significant digits and to an exponent of at
     least -6176, ties to even; @ref errc::invalid_number for a number other than zero that
     rounds to zero or past 9.999999999999999999999999999999999e6144;
     `boost::system::errc::invalid_argument` for a text that is no JSON number.
    */
    static boost::system::result<type> from_chars(std::string_view text) noexcept {
        return detail::decimal_from_chars<type>(text);
    }

    /**
     The number's exact decomposition.

     @param n The number.
     @return Its sign, its 34-digit coefficient and its exponent.
    */
    static decomposed decompose(type n) noexcept { return detail::decimal_decompose(n); }

    /**
     Whether the number is an integer.

     @param n The number.
     @return `true` when `n` is finite and has no fraction.
    */
    static bool is_integer(type n) noexcept { return detail::decimal_is_integer(n); }

    /**
     Writes the number's shortest text.

     @param out Where to write it.
     @param n The number.
     @return How many characters were written, or 0 when they do not fit.
    */
    static std::size_t to_chars(std::span<char> out, type n) noexcept {
        return detail::decimal_to_chars(out, n);
    }
};

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_NUMBERS_DECIMAL_HPP
