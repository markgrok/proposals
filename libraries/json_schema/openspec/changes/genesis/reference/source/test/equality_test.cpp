// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/detail/json/equal.hpp>
#include <webcpp/json_schema/detail/numbers/compare.hpp>
#include <webcpp/json_schema/detail/numbers/decompose.hpp>

#include "counting_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/monotonic_resource.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace js = webcpp::json_schema;

namespace {

bool eq(std::string_view a, std::string_view b) {
    std::pmr::monotonic_buffer_resource scratch;
    return js::detail::equal(boost::json::parse(a), boost::json::parse(b), scratch);
}

bool eqv(const boost::json::value& a, const boost::json::value& b) {
    std::pmr::monotonic_buffer_resource scratch;
    return js::detail::equal(a, b, scratch);
}

void values_compare_as_json_says() {
    BOOST_TEST(eq("1", "1.0"));
    BOOST_TEST(eq("-0.0", "0"));
    BOOST_TEST(!eq("1", "true"));
    BOOST_TEST(!eq("0", "false"));
    BOOST_TEST(!eq("null", "false"));
    BOOST_TEST(eq(R"({"a": 1, "b": [1, 2]})", R"({"b": [1.0, 2], "a": 1})"));
    BOOST_TEST(!eq("[1, 2]", "[2, 1]"));
    BOOST_TEST(!eq(R"({"a": 1})", R"({"a": 1, "b": 2})"));
    BOOST_TEST(!eq(R"("a")", R"("A")"));
    BOOST_TEST(eq("18446744073709551615", "18446744073709551615"));
    BOOST_TEST(!eq("9007199254740993", "9007199254740992"));
    BOOST_TEST(!eq("0.30000000000000004", "0.3"));
}

// Values of every kind, pairwise different, each equal to itself alone, in either order, also when
// parsed twice from its text.
void distinct_values_differ() {
    constexpr std::array<std::string_view, 27> texts{
        "null",
        "true",
        "false",
        "0",
        "1",
        "-1",
        "0.5",
        R"("")",
        R"("0")",
        R"("null")",
        R"("a\u0000b")",
        R"("a\u0000c")",
        R"("\ud83d\udc32")",
        "[]",
        "[null]",
        "[[]]",
        "[0]",
        "[0, 0]",
        "{}",
        R"({"": null})",
        R"({"a": null})",
        R"({"a": 0})",
        R"({"a": []})",
        R"({"a": {}})",
        R"({"a": 0, "b": 0})",
        R"({"b": 0, "c": 0})",
        R"([{"a": 0}])",
    };
    for (std::size_t i = 0; i < texts.size(); ++i) {
        for (std::size_t j = 0; j < texts.size(); ++j) {
            if (!BOOST_TEST_EQ(eq(texts[i], texts[j]), i == j)) {
                BOOST_TEST_EQ(texts[i], texts[j]);
            }
        }
    }
}

// Objects of more members than Boost.JSON looks up by a scan, in two orders.
void large_objects_ignore_order() {
    boost::json::object forward;
    boost::json::object backward;
    for (int i = 0; i < 200; ++i) {
        forward["k" + std::to_string(i)] = i;
        backward["k" + std::to_string(199 - i)] = 199 - i;
    }
    BOOST_TEST(eqv(forward, backward));
    backward["k7"] = 7.0;
    BOOST_TEST(eqv(forward, backward));
    backward["k7"] = 8;
    BOOST_TEST(!eqv(forward, backward));
    backward["k7"] = 7;
    backward.erase("k150");
    backward["k1500"] = 150;
    BOOST_TEST(!eqv(forward, backward));
}

// Numbers are equal exactly when the exact order of numbers says they are, whatever their kinds:
// int64, uint64 or double, the double read as the decimal of its shortest text.
void numbers_agree_with_the_exact_order() {
    const std::vector<boost::json::value> numbers{
        std::numeric_limits<std::int64_t>::min(),
        std::int64_t{-1},
        std::int64_t{0},
        std::int64_t{1},
        std::int64_t{9007199254740992},
        std::int64_t{9007199254740993},
        std::numeric_limits<std::int64_t>::max(),
        std::uint64_t{9223372036854775808ULL},
        std::uint64_t{9223372036854776000ULL},
        std::numeric_limits<std::uint64_t>::max(),
        -0.0,
        0.0,
        1.0,
        -1.0,
        0.1,
        0.1 + 0.2,
        0.3,
        1.5,
        9007199254740992.0,
        9007199254740994.0,
        9223372036854775808.0,
        -9223372036854775808.0,
        18446744073709551616.0,
        1e308,
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::denorm_min(),
    };
    for (const auto& a : numbers) {
        for (const auto& b : numbers) {
            const auto x = js::detail::decompose_value(a);
            const auto y = js::detail::decompose_value(b);
            if (!x || !y) {
                BOOST_ERROR("a finite number has no decomposition");
                return;
            }
            if (!BOOST_TEST_EQ(eqv(a, b), js::detail::compare(*x, *y) == 0)) {
                BOOST_TEST_EQ(a, b);
            }
        }
    }
}

// The same edges, each with its verdict written out, which does not depend on the exact order.
void numbers_at_their_edges() {
    const auto u = [](std::uint64_t v) { return boost::json::value(v); };
    const auto i = [](std::int64_t v) { return boost::json::value(v); };
    const auto d = [](double v) { return boost::json::value(v); };
    BOOST_TEST(eqv(i(1), d(1.0)));
    BOOST_TEST(eqv(i(0), d(-0.0)));
    BOOST_TEST(eqv(d(0.0), d(-0.0)));
    BOOST_TEST(eqv(i(1), u(1)));
    BOOST_TEST(!eqv(i(-1), u(18446744073709551615ULL)));
    BOOST_TEST(!eqv(i(std::numeric_limits<std::int64_t>::min()), u(9223372036854775808ULL)));
    BOOST_TEST(eqv(i(9007199254740992), d(9007199254740992.0)));
    BOOST_TEST(!eqv(i(9007199254740993), d(9007199254740992.0)));
    BOOST_TEST(!eqv(d(0.1 + 0.2), d(0.3)));
    BOOST_TEST(!eqv(u(18446744073709551615ULL), d(18446744073709551616.0)));
    BOOST_TEST(!eqv(i(9223372036854775807), d(9223372036854775808.0)));
    // A1: the double 2^63 is the decimal 9223372036854776000, its shortest text, not 2^63.
    BOOST_TEST(!eqv(u(9223372036854775808ULL), d(9223372036854775808.0)));
    BOOST_TEST(eqv(u(9223372036854776000ULL), d(9223372036854775808.0)));
    BOOST_TEST(!eqv(i(std::numeric_limits<std::int64_t>::min()), d(-9223372036854775808.0)));
    BOOST_TEST(eq("1e2", "100"));
    BOOST_TEST(eq("0.1e1", "1"));
    BOOST_TEST(!eq("1e-400", "1"));
}

// Boost.JSON holds what JSON cannot write: text past the largest double parses as an infinity.
// Each is equal to itself and to nothing else, a NaN to a NaN, so that equality stays an
// equivalence.
void non_finite_doubles_are_themselves() {
    constexpr double inf = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    BOOST_TEST(boost::json::parse("1e400").is_double());
    BOOST_TEST(eq("1e400", "1e400"));
    BOOST_TEST(eq("-1e400", "-1e400"));
    BOOST_TEST(!eq("1e400", "-1e400"));
    BOOST_TEST(!eq("1e400", "1.7976931348623157e308"));
    BOOST_TEST(!eq("1e400", "18446744073709551615"));
    BOOST_TEST(!eq("1e400", "null"));
    BOOST_TEST(eqv(boost::json::value(nan), boost::json::value(nan)));
    BOOST_TEST(!eqv(boost::json::value(nan), boost::json::value(inf)));
    BOOST_TEST(!eqv(boost::json::value(inf), boost::json::value(nan)));
    BOOST_TEST(!eqv(boost::json::value(nan), boost::json::value(0.0)));
    BOOST_TEST(!eqv(boost::json::value(nan), boost::json::value(std::int64_t{0})));
    BOOST_TEST(eqv(boost::json::array{inf, -inf, nan}, boost::json::array{inf, -inf, nan}));
}

// A chain of one-element arrays keeps one pair on the work stack at a time.
boost::json::value chain(boost::json::monotonic_resource& storage, std::size_t depth) {
    boost::json::value v = boost::json::array(&storage);
    for (std::size_t i = 0; i < depth; ++i) {
        boost::json::array outer(&storage);
        outer.push_back(std::move(v));
        v = std::move(outer);
    }
    return v;
}

// A chain of one-member objects around `leaf`.
boost::json::value object_chain(boost::json::monotonic_resource& storage, std::size_t depth,
                                boost::json::value leaf) {
    boost::json::value v(std::move(leaf), &storage);
    for (std::size_t i = 0; i < depth; ++i) {
        boost::json::object outer(&storage);
        outer["items"] = std::move(v);
        v = std::move(outer);
    }
    return v;
}

void depth_never_reaches_the_stack() {
    boost::json::monotonic_resource storage;
    std::pmr::monotonic_buffer_resource scratch;
    BOOST_TEST(js::detail::equal(chain(storage, 100000), chain(storage, 100000), scratch));
}

// Whether `a` equals the value `make` builds, in a storage of its own that is freed after.
template <class Make>
bool equals_built(const boost::json::value& a, Make make) {
    boost::json::monotonic_resource storage;
    std::pmr::monotonic_buffer_resource scratch;
    return js::detail::equal(a, make(storage), scratch);
}

// Nesting of 10^5 and 10^6 levels, equal, unequal at the very bottom, and of two depths.
void every_depth_is_walked_to_the_bottom() {
    for (const std::size_t depth : {std::size_t{100000}, std::size_t{1000000}}) {
        boost::json::monotonic_resource storage;
        const boost::json::value a = chain(storage, depth);
        BOOST_TEST(equals_built(a, [&](auto& s) { return chain(s, depth); }));
        BOOST_TEST(!equals_built(a, [&](auto& s) { return chain(s, depth - 1); }));
        BOOST_TEST(!equals_built(a, [&](auto& s) { return chain(s, depth + 1); }));
    }
    for (const std::size_t depth : {std::size_t{100000}, std::size_t{1000000}}) {
        boost::json::monotonic_resource storage;
        const boost::json::value one = object_chain(storage, depth, 1);
        BOOST_TEST(equals_built(one, [&](auto& s) { return object_chain(s, depth, 1.0); }));
        BOOST_TEST(!equals_built(one, [&](auto& s) { return object_chain(s, depth, 2); }));
        BOOST_TEST(!equals_built(one, [&](auto& s) { return object_chain(s, depth + 1, 1); }));
    }
}

void width_spills_into_the_given_resource() {
    boost::json::array wide;
    for (int i = 0; i < 1000; ++i) {
        wide.push_back(boost::json::array{i, i});
    }
    const boost::json::value a = wide;
    const boost::json::value b = wide;
    webcpp::test::counting_resource counted;
    BOOST_TEST(js::detail::equal(a, b, counted));
    BOOST_TEST(counted.allocations > 0);
    BOOST_TEST_EQ(counted.live, 0U);
    wide.back() = boost::json::array{0, 1};
    const boost::json::value c = wide;
    BOOST_TEST(!js::detail::equal(a, c, counted));
    BOOST_TEST_EQ(counted.live, 0U);
}

// A small value is compared on the inline stack alone.
void a_small_value_allocates_nothing() {
    const auto a = boost::json::parse(R"({"a": [1, 2, {"b": [true, null, "text"]}], "c": {}})");
    const auto b = boost::json::parse(R"({"c": {}, "a": [1.0, 2, {"b": [true, null, "text"]}]})");
    webcpp::test::counting_resource counted;
    BOOST_TEST(js::detail::equal(a, b, counted));
    BOOST_TEST_EQ(counted.allocations, 0U);
}

}  // namespace

int main() {
    values_compare_as_json_says();
    distinct_values_differ();
    large_objects_ignore_order();
    numbers_agree_with_the_exact_order();
    numbers_at_their_edges();
    non_finite_doubles_are_themselves();
    depth_never_reaches_the_stack();
    every_depth_is_walked_to_the_bottom();
    width_spills_into_the_given_resource();
    a_small_value_allocates_nothing();
    return boost::report_errors();
}
