// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The numbers against an exact oracle: numbers/oracle.txt, which numbers/oracle.py writes from
// Python's decimal and fractions, and whose format its Doc Comment states. Each line is one case,
// and a failure names its line.

#include <webcpp/json_schema/detail/numbers/compare.hpp>
#include <webcpp/json_schema/detail/numbers/decompose.hpp>
#include <webcpp/json_schema/detail/numbers/multiple_of.hpp>
#include <webcpp/json_schema/numbers/decimal.hpp>

#include "require.hpp"

#include <boost/core/lightweight_test.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

namespace js = webcpp::json_schema;
using d64 = boost::decimal::decimal64_t;
using d128 = boost::decimal::decimal128_t;

namespace {

/** The words of one line, separated by one space. */
struct words {
    std::array<std::string_view, 8> at{};
    std::size_t size = 0;
};

words split(std::string_view line) {
    words result;
    while (!line.empty() && result.size < result.at.size()) {
        const std::size_t space = line.find(' ');
        result.at.at(result.size++) = line.substr(0, space);
        line = space == std::string_view::npos ? std::string_view{} : line.substr(space + 1);
    }
    return result;
}

// The text of a number as the oracle writes it: [-]coefficient e exponent, the coefficient
// without trailing zeros, and zero as 0e0.
std::string text_of(const js::decomposed& d) {
    js::detail::u128 coefficient = js::detail::to_u128(d.coefficient);
    std::int64_t exponent = d.exponent;
    if (coefficient == 0U) {
        exponent = 0;
    }
    while (coefficient != 0U && coefficient % 10U == 0U) {
        coefficient /= 10U;
        ++exponent;
    }
    std::string digits;
    do {
        digits.insert(digits.begin(), static_cast<char>('0' + static_cast<int>(coefficient % 10U)));
        coefficient /= 10U;
    } while (coefficient != 0U);
    return (d.negative ? "-" : "") + digits + "e" + std::to_string(exponent);
}

template <class T>
std::optional<T> integer_of(std::string_view text, int base = 10) {
    T value{};
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (ec != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

std::optional<double> double_of(std::string_view bits) {
    const auto value = integer_of<std::uint64_t>(bits, 16);
    if (!value) {
        return std::nullopt;
    }
    return std::bit_cast<double>(*value);
}

template <class Number>
std::optional<js::decomposed> literal_of(std::string_view text) {
    const auto parsed = js::number_traits<Number>::from_chars(text);
    if (!parsed) {
        return std::nullopt;
    }
    return js::number_traits<Number>::decompose(*parsed);
}

// The operand of kind `kind` written `text`, or nothing when it is no number of that kind.
std::optional<js::decomposed> operand(std::string_view kind, std::string_view text) {
    if (kind == "d") {
        const auto d = double_of(text);
        return d ? js::detail::decompose_double(*d) : std::nullopt;
    }
    if (kind == "i") {
        const auto i = integer_of<std::int64_t>(text);
        return i ? js::detail::decompose(js::ir::number<d64>{std::in_place_index<0>, *i})
                 : std::optional<js::decomposed>{};
    }
    if (kind == "u") {
        const auto u = integer_of<std::uint64_t>(text);
        return u ? js::detail::decompose(js::ir::number<d64>{std::in_place_index<1>, *u})
                 : std::optional<js::decomposed>{};
    }
    if (kind == "s") {
        return literal_of<d64>(text);
    }
    if (kind == "S") {
        return literal_of<d128>(text);
    }
    return std::nullopt;
}

std::string_view order_of(std::strong_ordering o) {
    if (o < 0) {
        return "<";
    }
    return o == 0 ? "=" : ">";
}

/** The cases of each kind that were checked, and those that failed. */
struct tally {
    std::array<std::size_t, 4> checked{};
    std::size_t failed = 0;
};

// Reports a failure of the case on line `number`, `line`, which `held` says.
void check(tally& counts, bool held, std::size_t number, std::string_view line,
           std::string_view got) {
    if (!BOOST_TEST(held)) {
        ++counts.failed;
        std::cerr << "oracle.txt:" << number << ": " << line << " (got " << got << ")\n";
    }
}

template <class Number>
std::string loaded(double d) {
    const auto n = js::detail::schema_number_of_double<Number>(d);
    if (!n) {
        return "-";
    }
    const char* kinds = "iuN";
    return std::string(1, kinds[n->index()]) + " " + text_of(js::detail::decompose(*n));
}

// The order `b <=> a` when `a <=> b` is `order`.
std::string_view mirrored(std::string_view order) {
    if (order == "<") {
        return ">";
    }
    return order == ">" ? "<" : "=";
}

// An x line: the operand's exact value, or its refusal.
void check_exact(tally& counts, std::size_t number, std::string_view line, const words& w) {
    const auto value = operand(w.at[1], w.at[2]);
    const std::string got = value ? text_of(*value) : "-";
    check(counts, got == w.at[3], number, line, got);
    ++counts.checked[0];
}

// A c or an m line: the order of two operands, both ways, or whether one is a multiple.
void check_pair(tally& counts, std::size_t number, std::string_view line, const words& w) {
    const auto x = operand(w.at[1], w.at[2]);
    const auto y = operand(w.at[3], w.at[4]);
    if (!x || !y) {
        check(counts, false, number, line, "an operand refused");
        return;
    }
    if (w.at[0] == "c") {
        const std::string_view got = order_of(js::detail::compare(*x, *y));
        check(counts, got == w.at[5], number, line, got);
        const std::string_view back = order_of(js::detail::compare(*y, *x));
        check(counts, back == mirrored(w.at[5]), number, line, back);
        ++counts.checked[1];
        return;
    }
    const std::string_view got = js::detail::is_multiple_of(*x, *y) ? "1" : "0";
    check(counts, got == w.at[5], number, line, got);
    ++counts.checked[2];
}

// An n line: what the loader keeps of a double.
void check_loaded(tally& counts, std::size_t number, std::string_view line, const words& w) {
    const auto d = double_of(w.at[2]);
    if (!d) {
        check(counts, false, number, line, "no double");
        return;
    }
    const std::string got = w.at[1] == "s" ? loaded<d64>(*d) : loaded<d128>(*d);
    const std::string expected =
        w.size == 4 ? std::string(w.at[3]) : std::string(w.at[3]) + " " + std::string(w.at[4]);
    check(counts, got == expected, number, line, got);
    ++counts.checked[3];
}

void check_line(tally& counts, std::size_t number, std::string_view line) {
    const words w = split(line);
    const std::string_view op = w.at[0];
    if (op == "x" && w.size == 4) {
        check_exact(counts, number, line, w);
        return;
    }
    if ((op == "c" || op == "m") && w.size == 6) {
        check_pair(counts, number, line, w);
        return;
    }
    if (op == "n" && (w.size == 4 || w.size == 5)) {
        check_loaded(counts, number, line, w);
        return;
    }
    check(counts, false, number, line, "a line of no known form");
}

void the_oracle_agrees() {
    std::ifstream file(WEBCPP_TEST_JSON_SCHEMA_NUMBERS "/oracle.txt");
    if (!BOOST_TEST(file.is_open())) {
        return;
    }
    tally counts;
    std::string line;
    std::size_t number = 0;
    while (std::getline(file, line)) {
        ++number;
        if (line.empty() || line.front() == '#') {
            continue;
        }
        check_line(counts, number, line);
    }
    // Every kind of case is there, by the thousand for the comparisons and the multiples.
    BOOST_TEST_GE(counts.checked[0], 1000U);
    BOOST_TEST_GE(counts.checked[1], 4000U);
    BOOST_TEST_GE(counts.checked[2], 2000U);
    BOOST_TEST_GE(counts.checked[3], 500U);
    BOOST_TEST_EQ(counts.failed, 0U);
}

}  // namespace

int main() {
    the_oracle_agrees();
    return boost::report_errors();
}
