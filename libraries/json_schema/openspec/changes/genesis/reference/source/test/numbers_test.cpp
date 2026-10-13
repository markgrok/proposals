// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/detail/numbers/compare.hpp>
#include <webcpp/json_schema/detail/numbers/decompose.hpp>
#include <webcpp/json_schema/detail/numbers/multiple_of.hpp>
#include <webcpp/json_schema/errors.hpp>
#include <webcpp/json_schema/numbers/decimal.hpp>

#include "require.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/value.hpp>

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace js = webcpp::json_schema;
using d64 = boost::decimal::decimal64_t;
using d128 = boost::decimal::decimal128_t;

static_assert(js::number<d64>);
static_assert(js::number<d128>);
static_assert(!js::number<double>);  // no number_traits<double> ships: a double cannot hold 0.1

namespace {

using n64 = js::ir::number<d64>;
using n128 = js::ir::number<d128>;

js::decomposed schema(std::string_view text) {
    return js::detail::decompose(
        n64{std::in_place_index<2>, *js::number_traits<d64>::from_chars(text)});
}

js::decomposed schema_int(std::int64_t v) {
    return js::detail::decompose(n64{std::in_place_index<0>, v});
}

js::decomposed schema_uint(std::uint64_t v) {
    return js::detail::decompose(n64{std::in_place_index<1>, v});
}

// An instance double, which must be finite.
js::decomposed instance(double d) {
    const auto decomposed = js::detail::decompose_double(d);
    webcpp::test::require(BOOST_TEST(decomposed.has_value()));
    return decomposed.value_or(js::decomposed{});
}

// An instance value, which must be a number.
js::decomposed instance_value(const boost::json::value& v) {
    const auto decomposed = js::detail::decompose_value(v);
    webcpp::test::require(BOOST_TEST(decomposed.has_value()));
    return decomposed.value_or(js::decomposed{});
}

// A schema number of decimal128_t, from text that must be one.
js::decomposed schema128(std::string_view text) {
    const auto parsed = js::number_traits<d128>::from_chars(text);
    webcpp::test::require(BOOST_TEST(parsed.has_value()));
    return js::detail::decompose(n128{std::in_place_index<2>, *parsed});
}

// The 128-bit integer of decimal digits, which must fit.
js::detail::u128 u128_of(std::string_view digits) {
    js::detail::u128 value = 0U;
    for (const char c : digits) {
        value = value * 10U + static_cast<unsigned>(c - '0');
    }
    return value;
}

// The number coefficient * 10^exponent, its coefficient given as decimal digits.
js::decomposed exact(bool negative, std::string_view coefficient, std::int32_t exponent) {
    return {
        .negative = negative,
        .coefficient = js::detail::from_u128(u128_of(coefficient)),
        .exponent = exponent,
    };
}

void decimal_traits() {
    const js::decomposed tenth =
        js::number_traits<d64>::decompose(*js::number_traits<d64>::from_chars("0.1"));
    BOOST_TEST(
        js::detail::compare(
            tenth, {.negative = false, .coefficient = {.high = 0, .low = 1}, .exponent = -1}) == 0);
    BOOST_TEST(!js::number_traits<d64>::from_chars("1e400"));
    BOOST_TEST(js::number_traits<d128>::from_chars("1e400").has_value());
    BOOST_TEST(!js::number_traits<d64>::from_chars("1.5x"));
    BOOST_TEST(!js::number_traits<d64>::from_chars(""));
    BOOST_TEST(js::number_traits<d64>::is_integer(*js::number_traits<d64>::from_chars("2.0")));
    std::array<char, 32> text{};
    const std::size_t n =
        js::number_traits<d64>::to_chars(text, *js::number_traits<d64>::from_chars("1048503.5"));
    BOOST_TEST_EQ(std::string_view(text.data(), n), "1048503.5");
}

// What from_chars accepts is a JSON number's lexeme, as RFC 8259 writes one.
void decimal_traits_read_json_numbers_only() {
    const std::array<std::string_view, 17> malformed{
        "1e",  "1e+", "+1",   ".5",  "5.",   "01",    " 1",  "1 ",    "inf",
        "nan", "-",   "0x10", "--1", "1.e1", "1e1.5", "-.5", "1E+-1",
    };
    for (const std::string_view text : malformed) {
        BOOST_TEST(!js::number_traits<d64>::from_chars(text));
        BOOST_TEST(!js::number_traits<d128>::from_chars(text));
    }
    const std::array<std::string_view, 13> accepted{
        "0",     "-0",      "0.0",     "-0e5",   "1E+2",    "1e-2",
        "10",    "-1.5e-3", "0e-1000", "1.5E-3", "-0.5e+3", "123456789012345678901234567890",
        "0.000",
    };
    for (const std::string_view text : accepted) {
        BOOST_TEST(js::number_traits<d64>::from_chars(text).has_value());
        BOOST_TEST(js::number_traits<d128>::from_chars(text).has_value());
    }
}

// A number either type cannot hold is refused, never a zero for a number too small, nor a value
// beyond the type's largest; and the rest of the traits.
void decimal_traits_at_their_limits() {
    // decimal64_t: 16 digits, exponents down to -398 (subnormal) and up to 9.999...e384.
    BOOST_TEST(js::number_traits<d64>::from_chars("1e384").has_value());
    BOOST_TEST(js::number_traits<d64>::from_chars("9.999999999999999e384").has_value());
    BOOST_TEST(js::number_traits<d64>::from_chars("1e-398").has_value());
    BOOST_TEST(!js::number_traits<d64>::from_chars("1e385"));
    BOOST_TEST(!js::number_traits<d64>::from_chars("-1e385"));
    BOOST_TEST(!js::number_traits<d64>::from_chars("9.9999999999999995e384"));  // rounds past
    BOOST_TEST(!js::number_traits<d64>::from_chars("1e-399"));                  // would be zero
    BOOST_TEST(!js::number_traits<d64>::from_chars("-4e-400"));
    // decimal128_t: 34 digits, exponents from -6176 to 6144.
    BOOST_TEST(js::number_traits<d128>::from_chars("1e6144").has_value());
    BOOST_TEST(js::number_traits<d128>::from_chars("1e-6176").has_value());
    BOOST_TEST(!js::number_traits<d128>::from_chars("1e6145"));
    BOOST_TEST(!js::number_traits<d128>::from_chars("1e-6177"));
    // Integers.
    BOOST_TEST(!js::number_traits<d64>::is_integer(*js::number_traits<d64>::from_chars("2.5")));
    BOOST_TEST(js::number_traits<d64>::is_integer(*js::number_traits<d64>::from_chars("1e20")));
    BOOST_TEST(js::number_traits<d64>::is_integer(*js::number_traits<d64>::from_chars("-0")));
    BOOST_TEST(js::number_traits<d128>::is_integer(*js::number_traits<d128>::from_chars("4e1")));
    BOOST_TEST(!js::number_traits<d128>::is_integer(*js::number_traits<d128>::from_chars("4e-1")));
    // Text, and a buffer too small for it.
    std::array<char, 64> text{};
    const auto wide =
        *js::number_traits<d128>::from_chars("1234567890123456789012345678901234e-30");
    const std::size_t n = js::number_traits<d128>::to_chars(text, wide);
    BOOST_TEST_EQ(std::string_view(text.data(), n), "1234.567890123456789012345678901234");
    std::array<char, 4> small{};
    BOOST_TEST_EQ(js::number_traits<d128>::to_chars(small, wide), 0U);
}

// The decomposition holds the number exactly, its coefficient across both halves.
void decompositions_are_exact() {
    BOOST_TEST(schema128("1234567890123456789012345678901234e-30") ==
               (js::decomposed{.negative = false,
                               .coefficient = js::detail::from_u128(
                                   u128_of("1234567890123456789012345678901234")),
                               .exponent = -30}));
    BOOST_TEST(js::detail::compare(schema128("-1234567890123456789012345678901234e-30"),
                                   exact(true, "1234567890123456789012345678901234", -30)) == 0);
    BOOST_TEST(js::detail::compare(schema("-2.5e-3"), exact(true, "25", -4)) == 0);
    const js::detail::u128 big = u128_of("340282366920938463463374607431768211455");
    BOOST_TEST(js::detail::to_u128(js::detail::from_u128(big)) == big);
    BOOST_TEST_EQ(js::detail::from_u128(big).high, std::numeric_limits<std::uint64_t>::max());
    BOOST_TEST_EQ(js::detail::from_u128(big).low, std::numeric_limits<std::uint64_t>::max());
    BOOST_TEST_EQ(js::detail::from_u128(u128_of("18446744073709551616")).high, 1U);
    BOOST_TEST_EQ(js::detail::from_u128(u128_of("18446744073709551616")).low, 0U);
}

void instance_doubles_are_never_rounded() {
    const double two_to_64 = 18446744073709551616.0;
    BOOST_TEST(js::detail::compare(schema_uint(std::numeric_limits<std::uint64_t>::max()),
                                   instance(two_to_64)) < 0);
    BOOST_TEST(js::detail::compare(instance(0.1 + 0.2), schema("0.3")) > 0);
    BOOST_TEST(js::detail::compare(instance(0.1 + 0.2), instance(0.3)) > 0);
    BOOST_TEST(js::detail::compare(instance(1.1), schema("1.1")) == 0);  // exclusiveMinimum 1.1
    BOOST_TEST(js::detail::compare(instance(-0.0), schema_int(0)) == 0);
    BOOST_TEST(js::detail::compare(instance(1.0), schema_int(1)) == 0);
    BOOST_TEST(js::detail::compare(instance(1e-300), schema_int(0)) > 0);
    BOOST_TEST(js::detail::compare(schema_int(std::numeric_limits<std::int64_t>::min()),
                                   instance(-9.3e18)) > 0);
    BOOST_TEST(!js::detail::decompose_double(std::numeric_limits<double>::infinity()));
    BOOST_TEST(!js::detail::decompose_value(boost::json::value("1")));
}

// A double is the decimal its shortest round-trip text denotes, at the ends of its range too.
void the_extremes_of_a_double() {
    using limits = std::numeric_limits<double>;
    // The smallest subnormal: its text is 5e-324, its binary value 4.94...e-324.
    BOOST_TEST(js::detail::compare(instance(limits::denorm_min()), exact(false, "5", -324)) == 0);
    BOOST_TEST(js::detail::compare(instance(limits::denorm_min()), schema("4.95e-324")) > 0);
    BOOST_TEST(js::detail::compare(instance(limits::denorm_min()), schema_int(0)) > 0);
    BOOST_TEST(js::detail::compare(instance(-limits::denorm_min()), schema_int(0)) < 0);
    // The largest subnormal, and the smallest normal, of 17 digits.
    BOOST_TEST(js::detail::compare(instance(std::nextafter(limits::min(), 0.0)),
                                   exact(false, "2225073858507201", -323)) == 0);
    BOOST_TEST(
        js::detail::compare(instance(limits::min()), exact(false, "22250738585072014", -324)) == 0);
    // The largest double, whose 17 digits decimal64_t rounds and decimal128_t holds.
    BOOST_TEST(
        js::detail::compare(instance(limits::max()), exact(false, "17976931348623157", 292)) == 0);
    BOOST_TEST(js::detail::compare(instance(limits::max()), schema128("1.7976931348623157e308")) ==
               0);
    BOOST_TEST(js::detail::compare(instance(limits::max()), schema("1.7976931348623157e308")) < 0);
    BOOST_TEST(js::detail::compare(instance(-limits::max()), schema("-1e384")) > 0);
    // Negative zero is zero, and a multiple of anything.
    BOOST_TEST(js::detail::compare(instance(-0.0), instance(0.0)) == 0);
    BOOST_TEST(js::detail::compare(instance(-0.0), schema("-0")) == 0);
    BOOST_TEST(js::detail::compare(schema("-0"), schema_int(0)) == 0);
    BOOST_TEST(js::detail::is_multiple_of(instance(-0.0), schema("0.1")));
    // 17 digits, kept whole.
    BOOST_TEST(js::detail::compare(instance(0.30000000000000004),
                                   exact(false, "30000000000000004", -17)) == 0);
    BOOST_TEST(js::detail::compare(instance(0.1 + 0.2), schema128("0.30000000000000004")) == 0);
    BOOST_TEST(js::detail::compare(instance(0.1 + 0.2), schema("0.30000000000000004")) > 0);
    // A text written without an exponent, and one with a large one.
    BOOST_TEST(js::detail::compare(instance(123456789012345680000.0),
                                   exact(false, "123456789012345680000", 0)) == 0);
    BOOST_TEST(js::detail::compare(instance(18446744073709551616.0),
                                   exact(false, "18446744073709552", 3)) == 0);
    BOOST_TEST(js::detail::compare(instance(1e-7), exact(false, "1", -7)) == 0);
    BOOST_TEST(js::detail::compare(instance(-1.5e300), exact(true, "15", 299)) == 0);
}

// int64_t and uint64_t compare exactly with each other and with a double's decimal.
void integers_at_their_boundaries() {
    using i64 = std::numeric_limits<std::int64_t>;
    using u64 = std::numeric_limits<std::uint64_t>;
    const auto lowest = instance_value(boost::json::value(i64::min()));
    BOOST_TEST(lowest == exact(true, "9223372036854775808", 0));
    BOOST_TEST(js::detail::compare(lowest, schema_int(i64::min())) == 0);
    const auto highest = instance_value(boost::json::value(u64::max()));
    BOOST_TEST(js::detail::compare(highest, schema_uint(u64::max())) == 0);
    BOOST_TEST(js::detail::compare(highest, schema_int(i64::max())) > 0);
    BOOST_TEST(js::detail::compare(schema_int(-1), schema_uint(0)) < 0);
    // The double 2^63 is 9223372036854776000, its shortest text: above both integers near it.
    const double two_to_63 = 9223372036854775808.0;
    BOOST_TEST(js::detail::compare(schema_int(i64::max()), instance(two_to_63)) < 0);
    BOOST_TEST(js::detail::compare(schema_uint(9223372036854775808U), instance(two_to_63)) < 0);
    BOOST_TEST(js::detail::compare(schema_int(i64::min()), instance(-two_to_63)) > 0);
    // The double below 2^64 is 18446744073709550000, under uint64_t's largest.
    BOOST_TEST(js::detail::compare(schema_uint(u64::max()),
                                   instance(std::nextafter(18446744073709551616.0, 0.0))) > 0);
    // 2^64 - 1 = 3 * 5 * 17 * 257 * 641 * 65537 * 6700417.
    BOOST_TEST(js::detail::is_multiple_of(schema_uint(u64::max()), schema_int(6700417)));
    BOOST_TEST(!js::detail::is_multiple_of(schema_uint(u64::max()), schema_int(7)));
    BOOST_TEST(js::detail::is_multiple_of(schema_uint(u64::max()), schema("0.5")));
    BOOST_TEST(js::detail::is_multiple_of(schema_uint(u64::max()), schema("1e-8")));
    BOOST_TEST(
        js::detail::is_multiple_of(schema_uint(u64::max()), schema128("1.8446744073709551615e19")));
    BOOST_TEST(!js::detail::is_multiple_of(schema_uint(u64::max()), schema_uint(u64::max() - 1)));
    BOOST_TEST(js::detail::is_multiple_of(schema_int(i64::min()), schema_int(2)));
    BOOST_TEST(
        js::detail::is_multiple_of(schema_int(i64::min()), schema_uint(9223372036854775808U)));
}

// A literal at the ends of a Number's exponents compares, and divides, exactly.
void huge_exponents_in_schema_literals() {
    using limits = std::numeric_limits<double>;
    BOOST_TEST(js::detail::compare(schema("1e384"), instance(limits::max())) > 0);
    BOOST_TEST(js::detail::compare(schema("1e-398"), instance(limits::denorm_min())) < 0);
    BOOST_TEST(js::detail::compare(schema("1e-398"), schema_int(0)) > 0);
    BOOST_TEST(js::detail::compare(schema128("1e6144"), instance(limits::max())) > 0);
    BOOST_TEST(js::detail::compare(schema128("-1e6144"), schema("-1e384")) < 0);
    BOOST_TEST(js::detail::compare(schema128("1e-6176"), instance(limits::denorm_min())) < 0);
    BOOST_TEST(js::detail::compare(schema128("1e-6176"), schema128("2e-6176")) < 0);
    BOOST_TEST(js::detail::is_multiple_of(instance(1e308), schema128("1e-6176")));
    BOOST_TEST(js::detail::is_multiple_of(instance(1.5), schema128("1e-6176")));
    BOOST_TEST(js::detail::is_multiple_of(instance(limits::denorm_min()), schema("1e-398")));
    BOOST_TEST(!js::detail::is_multiple_of(instance(limits::denorm_min()), schema("3e-398")));
    BOOST_TEST(!js::detail::is_multiple_of(instance(1e308), schema128("1e6144")));
    BOOST_TEST(js::detail::is_multiple_of(schema128("1e6144"), instance(1e308)));
    BOOST_TEST(!js::detail::is_multiple_of(schema128("1e6144"), schema_int(3)));
    BOOST_TEST(js::detail::is_multiple_of(schema128("3e6144"), schema_int(3)));
    BOOST_TEST(js::detail::is_multiple_of(schema128("9.999999999999999999999999999999999e6144"),
                                          schema128("1e6111")));
}

void the_schema_side_keeps_its_documented_limit() {
    // 17 digits round in decimal64_t (16), not in decimal128_t (34)
    const auto rounded = *js::number_traits<d64>::from_chars("0.30000000000000004");
    BOOST_TEST(rounded == *js::number_traits<d64>::from_chars("0.3"));
    const auto wide = *js::number_traits<d128>::from_chars("0.30000000000000004");
    BOOST_TEST(wide != *js::number_traits<d128>::from_chars("0.3"));
}

void multiple_of_is_exact() {
    using js::detail::is_multiple_of;
    // multipleOf.json of draft 2020-12, its required cases
    BOOST_TEST(is_multiple_of(schema_int(10), schema_int(2)));
    BOOST_TEST(!is_multiple_of(schema_int(7), schema_int(2)));
    BOOST_TEST(is_multiple_of(schema_int(0), schema("1.5")));
    BOOST_TEST(is_multiple_of(instance(4.5), schema("1.5")));
    BOOST_TEST(!is_multiple_of(schema_int(35), schema("1.5")));
    BOOST_TEST(is_multiple_of(instance(0.0075), schema("0.0001")));
    BOOST_TEST(!is_multiple_of(instance(0.00751), schema("0.0001")));
    BOOST_TEST(!is_multiple_of(instance(1e308), schema("0.123456789")));
    BOOST_TEST(is_multiple_of(schema_int(12391239123), schema("1e-8")));
    // optional/float-overflow.json, and 1e308 by 0.0001
    BOOST_TEST(is_multiple_of(instance(1e308), schema("0.5")));
    BOOST_TEST(is_multiple_of(instance(1e308), schema("0.0001")));
    // a coefficient of 34 digits in decimal128
    using n128 = js::ir::number<d128>;
    const auto y = js::detail::decompose(
        n128{std::in_place_index<2>,
             *js::number_traits<d128>::from_chars("1234567890123456789012345678901234e-30")});
    const auto x = js::detail::decompose(
        n128{std::in_place_index<2>,
             *js::number_traits<d128>::from_chars("3703703670370370367037037036703702e-30")});
    BOOST_TEST(is_multiple_of(x, y));
    BOOST_TEST(!is_multiple_of(y, x));
}

// Divisors of up to 39 digits, and quotients of hundreds, without overflow or floating point.
void multiple_of_large_divisors_and_quotients() {
    using js::detail::is_multiple_of;
    // 1e308 / 2.5e-300 = 4e607; 1e308 / 7e-300 is no integer.
    BOOST_TEST(is_multiple_of(instance(1e308), schema("2.5e-300")));
    BOOST_TEST(!is_multiple_of(instance(1e308), schema("7e-300")));
    BOOST_TEST(!is_multiple_of(instance(1e308), schema128("1234567890123456789012345678901234")));
    BOOST_TEST(is_multiple_of(schema128("1234567890123456789012345678901234e300"),
                              schema128("1234567890123456789012345678901234e-300")));
    BOOST_TEST(!is_multiple_of(schema128("1234567890123456789012345678901233e300"),
                               schema128("1234567890123456789012345678901234e-300")));
    // A divisor whose coefficient needs the shift: 0.0001 into 0.00003 and into 0.0003.
    BOOST_TEST(!is_multiple_of(schema("0.00003"), schema("0.0001")));
    BOOST_TEST(is_multiple_of(schema("0.0003"), schema("0.0001")));
    BOOST_TEST(!is_multiple_of(schema("1e-5"), schema("1e-4")));
    BOOST_TEST(is_multiple_of(schema_int(10), schema("1e1")));
    BOOST_TEST(!is_multiple_of(schema_int(10), schema("1e2")));
    // 5^55, of 39 digits, is above 2^127: the modular arithmetic must not overflow. 10^55 is
    // 5^55 * 2^55; 10^54 is no multiple of it, nor is 7 * 10^54.
    const js::decomposed five_to_55 = exact(false, "277555756156289135105907917022705078125", 0);
    BOOST_TEST(is_multiple_of(exact(false, "1", 55), five_to_55));
    BOOST_TEST(is_multiple_of(exact(false, "3", 57), five_to_55));
    BOOST_TEST(!is_multiple_of(exact(false, "1", 54), five_to_55));
    BOOST_TEST(is_multiple_of(exact(false, "7", 300), five_to_55));
    BOOST_TEST(!is_multiple_of(exact(false, "7", 54), five_to_55));
    // 2^128 - 1 as a coefficient: 4e38 exceeds it, so it is no divisor of it.
    const js::decomposed most = exact(false, "340282366920938463463374607431768211455", 0);
    BOOST_TEST(is_multiple_of(most, most));
    BOOST_TEST(is_multiple_of(exact(false, "340282366920938463463374607431768211455", 1), most));
    BOOST_TEST(!is_multiple_of(exact(false, "1", 0), most));
    BOOST_TEST(!is_multiple_of(most, exact(false, "4", 38)));
    BOOST_TEST(!is_multiple_of(most, exact(false, "1", 39)));
    BOOST_TEST(is_multiple_of(most, exact(false, "5", 0)));
}

// Numbers beyond any Number's coefficient still compare exactly, without overflow.
void comparisons_beyond_any_number() {
    const js::decomposed most = exact(false, "340282366920938463463374607431768211455", 0);
    BOOST_TEST(js::detail::compare(most, exact(false, "4", 38)) < 0);
    BOOST_TEST(js::detail::compare(exact(false, "4", 38), most) > 0);
    BOOST_TEST(js::detail::compare(exact(true, "4", 38),
                                   exact(true, "340282366920938463463374607431768211455", 0)) < 0);
    BOOST_TEST(js::detail::compare(most, exact(false, "3", 38)) > 0);
    BOOST_TEST(js::detail::compare(most, most) == 0);
    BOOST_TEST(
        js::detail::compare(most, exact(false, "340282366920938463463374607431768211455", 1)) < 0);
    BOOST_TEST(
        js::detail::compare(exact(false, "34028236692093846346337460743176821145", 1), most) < 0);
    BOOST_TEST(
        js::detail::compare(exact(false, "1", 2147483647), exact(false, "9", -2147483647 - 1)) > 0);
    BOOST_TEST(js::detail::compare(exact(true, "0", 5), exact(false, "0", -5)) == 0);
    BOOST_TEST(js::detail::compare(exact(false, "100", -2), exact(false, "1", 0)) == 0);
}

void integers_by_value() {
    BOOST_TEST(js::detail::is_integer_value(boost::json::value(1.0), true));
    BOOST_TEST(!js::detail::is_integer_value(boost::json::value(1.0), false));  // draft 4
    BOOST_TEST(js::detail::is_integer_value(boost::json::value(std::int64_t{1}), false));
    BOOST_TEST(!js::detail::is_integer_value(boost::json::value(1.5), true));
    const auto big = js::detail::schema_number_of_double<d64>(1e20);
    BOOST_TEST(big && big->index() == 2);
    const auto one = js::detail::schema_number_of_double<d64>(1.0);
    BOOST_TEST(one && one->index() == 0);
}

void integers_by_value_at_the_edges() {
    using limits = std::numeric_limits<double>;
    BOOST_TEST(js::detail::is_integer_value(
        boost::json::value(std::numeric_limits<std::uint64_t>::max()), false));
    BOOST_TEST(js::detail::is_integer_value(boost::json::value(1e300), true));
    BOOST_TEST(js::detail::is_integer_value(boost::json::value(-0.0), true));
    BOOST_TEST(!js::detail::is_integer_value(boost::json::value(limits::denorm_min()), true));
    BOOST_TEST(!js::detail::is_integer_value(boost::json::value(limits::quiet_NaN()), true));
    BOOST_TEST(!js::detail::is_integer_value(boost::json::value(limits::infinity()), true));
    BOOST_TEST(!js::detail::is_integer_value(boost::json::value("1"), true));
    BOOST_TEST(!js::detail::is_integer_value(boost::json::value(true), true));
    BOOST_TEST(!js::detail::decompose_value(boost::json::value(nullptr)));
    BOOST_TEST(!js::detail::decompose_double(limits::quiet_NaN()));
    BOOST_TEST(!js::detail::decompose_double(-limits::infinity()));
}

// A schema double is the decimal of its shortest text, as an instance double is, so the same
// double on both sides compares equal: an integer when that decimal is one that fits.
void schema_doubles_are_their_shortest_text() {
    using limits = std::numeric_limits<double>;
    const auto minus_zero = js::detail::schema_number_of_double<d64>(-0.0);
    BOOST_TEST(minus_zero && *minus_zero == (n64{std::in_place_index<0>, 0}));
    // 2^60 is 1152921504606846976, its shortest text 1.152921504606847e18.
    const auto two_to_60 = js::detail::schema_number_of_double<d64>(1152921504606846976.0);
    BOOST_TEST(two_to_60 && *two_to_60 == (n64{std::in_place_index<0>, 1152921504606847000}));
    // 2^63's text, 9223372036854776000, fits uint64_t only; -2^63's fits neither.
    const auto two_to_63 = js::detail::schema_number_of_double<d64>(9223372036854775808.0);
    BOOST_TEST(two_to_63 && *two_to_63 == (n64{std::in_place_index<1>, 9223372036854776000U}));
    const auto minus_two_to_63 = js::detail::schema_number_of_double<d64>(-9223372036854775808.0);
    BOOST_TEST(minus_two_to_63 && minus_two_to_63->index() == 2);
    const auto half = js::detail::schema_number_of_double<d64>(1.5);
    BOOST_TEST(half &&
               *half == (n64{std::in_place_index<2>, *js::number_traits<d64>::from_chars("1.5")}));
    BOOST_TEST(!js::detail::schema_number_of_double<d64>(limits::infinity()));
    BOOST_TEST(!js::detail::schema_number_of_double<d64>(limits::quiet_NaN()));
    const std::array<double, 14> doubles{
        0.0,
        -0.0,
        1.0,
        0.1,
        0.1 + 0.2,
        1.1,
        1e20,
        1e300,
        -1e-7,
        limits::max(),
        limits::min(),
        limits::denorm_min(),
        1152921504606846976.0,
        18446744073709551616.0,
    };
    for (const double d : doubles) {
        const auto wide = js::detail::schema_number_of_double<d128>(d);
        BOOST_TEST(wide.has_value());
        BOOST_TEST(js::detail::compare(js::detail::decompose(wide.value_or(n128{})), instance(d)) ==
                   0);
    }
}

// The integer of a decimal: int64_t up to 2^63 - 1 and down to -2^63, uint64_t up to 2^64 - 1.
void integers_of_decimals() {
    using js::detail::integer_of;
    const auto int64_of = [](const js::decomposed& d) {
        const auto n = integer_of<d64>(d);
        return n && n->index() == 0 ? std::optional<std::int64_t>(std::get<0>(*n)) : std::nullopt;
    };
    const auto uint64_of = [](const js::decomposed& d) {
        const auto n = integer_of<d64>(d);
        return n && n->index() == 1 ? std::optional<std::uint64_t>(std::get<1>(*n)) : std::nullopt;
    };
    BOOST_TEST(int64_of(exact(false, "9223372036854775807", 0)) ==
               std::numeric_limits<std::int64_t>::max());
    BOOST_TEST(uint64_of(exact(false, "9223372036854775808", 0)) == 9223372036854775808U);
    BOOST_TEST(int64_of(exact(true, "9223372036854775808", 0)) ==
               std::numeric_limits<std::int64_t>::min());
    BOOST_TEST(!integer_of<d64>(exact(true, "9223372036854775809", 0)));
    BOOST_TEST(uint64_of(exact(false, "18446744073709551615", 0)) ==
               std::numeric_limits<std::uint64_t>::max());
    BOOST_TEST(!integer_of<d64>(exact(false, "18446744073709551616", 0)));
    BOOST_TEST(!integer_of<d64>(exact(false, "1", 20)));
    BOOST_TEST(int64_of(exact(false, "1", 18)) == 1000000000000000000);
    BOOST_TEST(!integer_of<d64>(exact(false, "15", -1)));
    BOOST_TEST(int64_of(exact(false, "100", -2)) == 1);
    BOOST_TEST(int64_of(exact(true, "0", 5)) == 0);
}

// Whether the literal reads, in Number, as the same number as `expected` does.
template <class Number>
bool reads_as(std::string_view text, std::string_view expected) {
    const auto got = js::number_traits<Number>::from_chars(text);
    const auto want = js::number_traits<Number>::from_chars(expected);
    if (!got || !want) {
        return false;
    }
    return js::detail::compare(js::number_traits<Number>::decompose(*got),
                               js::number_traits<Number>::decompose(*want)) == 0;
}

// Whether the literal is refused as a number Number cannot hold.
template <class Number>
bool cannot_hold(std::string_view text) {
    const auto got = js::number_traits<Number>::from_chars(text);
    return !got && got.error() == js::errc::invalid_number;
}

// A literal of any length is read whole: its zeros, however many, and its digits past the
// Number's precision, which round correctly, ties to even.
void literals_of_any_length() {
    // Zeros with exponents past any type's, and past 64 bits.
    const std::array<std::string_view, 5> zeros{
        "0e-100000",
        "-0e99999999999",
        "0e2147483648",
        "0.000e+99999999999999999999",
        "-0.0e-99999999999999999999",
    };
    for (const std::string_view zero : zeros) {
        BOOST_TEST(reads_as<d64>(zero, "0"));
        BOOST_TEST(reads_as<d128>(zero, "0"));
    }
    const auto minus_zero = js::number_traits<d64>::from_chars("-0e99999999999");
    BOOST_TEST(minus_zero && boost::decimal::signbit(*minus_zero));
    // One, written with 100,000 zeros after it, or before it.
    const std::string one = "1" + std::string(100000, '0') + "e-100000";
    BOOST_TEST(reads_as<d64>(one, "1"));
    BOOST_TEST(reads_as<d128>(one, "1"));
    const std::string tenth = "0." + std::string(100000, '0') + "1e100001";
    BOOST_TEST(reads_as<d64>(tenth, "1"));
    BOOST_TEST(reads_as<d128>(tenth, "1"));
    BOOST_TEST(cannot_hold<d64>("1" + std::string(100000, '0') + "1"));
    // decimal64_t: a tie at the 17th digit goes to even, anything past it decides.
    BOOST_TEST(reads_as<d64>("1.0000000000000005", "1"));
    BOOST_TEST(reads_as<d64>("1.00000000000000050001", "1.000000000000001"));
    BOOST_TEST(reads_as<d64>("1.0000000000000005000000000000000000000001", "1.000000000000001"));
    BOOST_TEST(reads_as<d64>("1.0000000000000004999999999999999999999999", "1"));
    BOOST_TEST(reads_as<d64>("1.0000000000000015", "1.000000000000002"));
    BOOST_TEST(reads_as<d64>("99999999999999995", "1e17"));
    // decimal128_t: the same at the 35th digit.
    const std::string tie128 = "1." + std::string(33, '0') + "5";
    const std::string next128 = "1." + std::string(32, '0') + "1";
    BOOST_TEST(reads_as<d128>(tie128, "1"));
    BOOST_TEST(reads_as<d128>(tie128 + "0001", next128));
    BOOST_TEST(reads_as<d128>(tie128 + std::string(60, '0') + "1", next128));
    // Exponents past 64 bits.
    BOOST_TEST(cannot_hold<d64>("1e99999999999999999999"));
    BOOST_TEST(cannot_hold<d64>("1e-99999999999999999999"));
    BOOST_TEST(cannot_hold<d128>("-1e99999999999999999999"));
}

// The ends of each type: half the smallest subnormal rounds to zero, ties to even, and is refused;
// anything above it is the smallest; a value that rounds past the largest is refused.
void literals_at_the_ends_of_each_type() {
    BOOST_TEST(cannot_hold<d64>("5e-399"));
    BOOST_TEST(cannot_hold<d64>("-5e-399"));
    BOOST_TEST(cannot_hold<d64>("4.9999999999999999999999e-399"));
    BOOST_TEST(reads_as<d64>("5.0000000000000000000001e-399", "1e-398"));
    BOOST_TEST(reads_as<d64>("1.5e-398", "2e-398"));
    BOOST_TEST(reads_as<d64>("2.5e-398", "2e-398"));
    BOOST_TEST(reads_as<d64>("1.2345678901234565e-385", "1.2345678901235e-385"));
    BOOST_TEST(reads_as<d64>("1.23456789012345650001e-390", "1.23456789e-390"));
    BOOST_TEST(cannot_hold<d64>("9.9999999999999995e384"));
    BOOST_TEST(cannot_hold<d64>("-9.9999999999999995e384"));
    BOOST_TEST(reads_as<d64>("9.99999999999999949999999999e384", "9.999999999999999e384"));
    BOOST_TEST(cannot_hold<d128>("5e-6177"));
    BOOST_TEST(reads_as<d128>("5.000000000000000000000000000000000000000001e-6177", "1e-6176"));
    const std::string nines128 = "9." + std::string(33, '9');
    BOOST_TEST(cannot_hold<d128>(nines128 + "5e6144"));
    BOOST_TEST(reads_as<d128>(nines128 + "49999e6144", nines128 + "e6144"));
}

// A number the type cannot hold is the library's invalid_number; text that is no JSON number is
// invalid_argument.
void refusals_name_their_reason() {
    for (const std::string_view text : {"1e400", "1e-399", "9.9999999999999995e384"}) {
        const auto refused = js::number_traits<d64>::from_chars(text);
        if (!BOOST_TEST(!refused)) {
            continue;
        }
        BOOST_TEST(refused.error() == js::errc::invalid_number);
        BOOST_TEST_EQ(refused.error().message(), "the number cannot be represented");
        const boost::system::error_code code = refused.error();
        BOOST_TEST_EQ(std::string_view(code.category().name()), "webcpp.json_schema");
    }
    const auto malformed = js::number_traits<d128>::from_chars("1e");
    BOOST_TEST(!malformed && malformed.error() == boost::system::errc::make_error_code(
                                                      boost::system::errc::invalid_argument));
}

}  // namespace

int main() {
    decimal_traits();
    decimal_traits_read_json_numbers_only();
    decimal_traits_at_their_limits();
    decompositions_are_exact();
    instance_doubles_are_never_rounded();
    the_extremes_of_a_double();
    integers_at_their_boundaries();
    huge_exponents_in_schema_literals();
    the_schema_side_keeps_its_documented_limit();
    multiple_of_is_exact();
    multiple_of_large_divisors_and_quotients();
    comparisons_beyond_any_number();
    integers_by_value();
    integers_by_value_at_the_edges();
    schema_doubles_are_their_shortest_text();
    integers_of_decimals();
    literals_of_any_length();
    literals_at_the_ends_of_each_type();
    refusals_name_their_reason();
    return boost::report_errors();
}
