// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// JSON text read exactly: the exact Number of each lexeme whose double denotes another, keyed by
// its JSON Pointer, the double compared by its shortest text; the last of duplicate keys in the
// value and in the table; refusals as values, the depth bounded; every allocation in the given
// allocator, none in the default resource, the table's memory linear in the text however long
// its keys; a parsed document moved and destroyed safely; and the table's cost under keys chosen
// to collide, bounded well under quadratic.

#include <webcpp/json_schema/json/parse.hpp>

#include "counting_resource.hpp"
#include "recording_resource.hpp"

#include <boost/container_hash/hash.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/json/error.hpp>
#include <boost/json/parse_options.hpp>
#include <boost/json/value.hpp>

#include <cstddef>
#include <memory_resource>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace js = webcpp::json_schema;
using d64 = boost::decimal::decimal64_t;
using d128 = boost::decimal::decimal128_t;

namespace {

struct wide_traits : js::default_traits {
    using number_type = d128;
};

struct recording_traits : js::default_traits {
    using allocator_type = webcpp::test::recording_allocator<std::byte>;
};

d64 dec(std::string_view text) {
    return *js::number_traits<d64>::from_chars(text);
}

d128 wide(std::string_view text) {
    return *js::number_traits<d128>::from_chars(text);
}

// Sets the default resource for its lifetime, and puts the previous one back.
class default_resource_guard {
public:
    explicit default_resource_guard(std::pmr::memory_resource* r)
        : previous_(std::pmr::set_default_resource(r)) {}

    default_resource_guard(const default_resource_guard&) = delete;
    default_resource_guard& operator=(const default_resource_guard&) = delete;

    ~default_resource_guard() { std::pmr::set_default_resource(previous_); }

private:
    std::pmr::memory_resource* previous_;
};

void parse_keeps_what_a_double_loses() {
    auto parsed = js::parse(R"({"a": [0.1, 9007199254740993.0], "b": 12, "c": 1e400})");
    if (!BOOST_TEST(parsed.has_value())) {
        return;
    }
    BOOST_TEST(parsed->exact_numbers.find("/a/0") == nullptr);  // the double's text says 0.1
    const auto* exact = parsed->exact_numbers.find("/a/1");     // the double holds ...992
    if (BOOST_TEST(exact != nullptr)) {
        BOOST_TEST(exact->representable);
        BOOST_TEST(exact->value == dec("9007199254740993"));
    }
    BOOST_TEST(parsed->exact_numbers.find("/b") == nullptr);
    const auto* huge = parsed->exact_numbers.find("/c");  // the double is infinite
    BOOST_TEST(huge != nullptr && !huge->representable);
    auto wide_parsed = js::parse<wide_traits>("[12345678901234567890123.5]");
    const auto* big = wide_parsed->exact_numbers.find("/0");
    BOOST_TEST(big != nullptr &&
               big->value == *js::number_traits<d128>::from_chars("12345678901234567890123.5"));
}

// The double is compared by its shortest text, never its fixed notation, which spells every digit
// of its binary value: 2^60 written with a fraction is 1152921504606846976 in the text, and
// 1.152921504606847e+18 is the decimal the double denotes. decimal128_t holds both apart; in
// decimal64_t both round to 1.152921504606847e+18.
void the_double_is_its_shortest_text() {
    const auto in_wide = js::parse<wide_traits>("[1152921504606846976.0, 1.152921504606847e18]");
    if (!BOOST_TEST(in_wide.has_value())) {
        return;
    }
    const auto* exact = in_wide->exact_numbers.find("/0");
    if (BOOST_TEST(exact != nullptr)) {
        BOOST_TEST(exact->representable);
        BOOST_TEST(exact->value == wide("1152921504606846976"));
        BOOST_TEST_EQ(exact->read_as, 1152921504606846976.0);
    }
    BOOST_TEST(in_wide->exact_numbers.find("/1") == nullptr);
    const auto in_d64 = js::parse("[1152921504606846976.0]");
    BOOST_TEST(in_d64->exact_numbers.empty());
}

void the_last_duplicate_wins_in_the_table_too() {
    auto parsed = js::parse(R"({"maximum": 9007199254740993.0, "maximum": 5.5})");
    BOOST_TEST_EQ(parsed->value.at("maximum"), 5.5);
    const auto* stale = parsed->exact_numbers.find("/maximum");
    BOOST_TEST(stale == nullptr || stale->read_as != parsed->value.at("maximum").as_double());
}

// A later duplicate whose double is the same as the earlier one's, but whose text is that double's
// own: the earlier exact Number must not be read for it.
void a_duplicate_with_the_same_double_drops_the_entry() {
    const auto parsed = js::parse(
        R"({"m": 9007199254740993.0, "m": 9007199254740992.0,
            "n": {"x": 9007199254740993.0}, "n": {"x": 9007199254740992},
            "o": [1e400], "o": [9007199254740993.0]})");
    if (!BOOST_TEST(parsed.has_value())) {
        return;
    }
    BOOST_TEST_EQ(parsed->value.at("m").as_double(), 9007199254740992.0);
    BOOST_TEST(parsed->exact_numbers.find("/m") == nullptr);
    BOOST_TEST(parsed->exact_numbers.find("/n/x") == nullptr);
    const auto* replaced = parsed->exact_numbers.find("/o/0");
    if (BOOST_TEST(replaced != nullptr)) {
        BOOST_TEST(replaced->representable);
        BOOST_TEST(replaced->value == dec("9007199254740993"));
    }
}

void pointers_are_rfc_6901() {
    const auto parsed = js::parse(
        R"([[1, 2], [3, 9007199254740993.0], {"a/b": {"~": -9007199254740993.0, "": 1e400}}])");
    if (!BOOST_TEST(parsed.has_value())) {
        return;
    }
    BOOST_TEST(parsed->exact_numbers.find("/1/1") != nullptr);
    const auto* escaped = parsed->exact_numbers.find("/2/a~1b/~0");
    if (BOOST_TEST(escaped != nullptr)) {
        BOOST_TEST(escaped->value == dec("-9007199254740993"));
    }
    BOOST_TEST(parsed->exact_numbers.find("/2/a~1b/") != nullptr);  // the empty key
    BOOST_TEST(parsed->exact_numbers.find("/2/a/b/~") == nullptr);
    BOOST_TEST(parsed->exact_numbers.find("/2/a~1b/~") == nullptr);
    BOOST_TEST(parsed->exact_numbers.find("/01/1") == nullptr);
    BOOST_TEST(parsed->exact_numbers.find("1/1") == nullptr);
    BOOST_TEST(parsed->exact_numbers.find("/1/1/") == nullptr);
    BOOST_TEST(parsed->exact_numbers.find("/1") == nullptr);
    BOOST_TEST(parsed->exact_numbers.find("") == nullptr);
    BOOST_TEST(parsed->exact_numbers.find("/") == nullptr);
    const auto empty_key = js::parse(R"({"": [1e400]})");
    BOOST_TEST(empty_key->exact_numbers.find("//0") != nullptr);
    BOOST_TEST(empty_key->exact_numbers.find("x/0") == nullptr);
    BOOST_TEST(empty_key->exact_numbers.find("/0") == nullptr);
    const auto root = js::parse("1e400");
    BOOST_TEST(root->exact_numbers.find("") != nullptr);
    BOOST_TEST(root->exact_numbers.find("/") == nullptr);
}

void most_documents_record_nothing() {
    const auto parsed =
        js::parse(R"({"a": [0.1, 0.2, 0.30000000000000004, 1e300, -0.0, 5e-324, 12, -3]})");
    if (!BOOST_TEST(parsed.has_value())) {
        return;
    }
    BOOST_TEST(parsed->exact_numbers.empty());
    BOOST_TEST(parsed->exact_numbers.find("/a/0") == nullptr);
}

void the_options_given_are_kept_but_the_numbers() {
    boost::json::parse_options options;
    options.numbers = boost::json::number_precision::none;
    options.allow_comments = true;
    options.allow_trailing_commas = true;
    const auto parsed = js::parse("[0.1, 9007199254740993.0, /* a comment */]", {}, options);
    if (!BOOST_TEST(parsed.has_value())) {
        return;
    }
    BOOST_TEST_EQ(parsed->value.at(0).as_double(), 0.1);
    BOOST_TEST(parsed->exact_numbers.find("/1") != nullptr);
    BOOST_TEST(js::parse("[1, /* a comment */]").has_error());
}

void refusals_are_values() {
    BOOST_TEST(js::parse("{} x").has_error());
    BOOST_TEST(js::parse("{\"a\": ").has_error());
    const std::string deep(100000, '[');
    const auto too_deep = js::parse(deep);
    BOOST_TEST(too_deep.has_error() && too_deep.error().code == boost::json::error::too_deep);
    boost::json::parse_options asked;
    asked.max_depth = 1000000;
    BOOST_TEST(js::parse(deep, {}, asked).error().code == boost::json::error::too_deep);
}

void refusals_say_where() {
    const auto extra = js::parse("{} x");
    if (BOOST_TEST(extra.has_error())) {
        BOOST_TEST(extra.error().code == boost::json::error::extra_data);
        BOOST_TEST_EQ(std::string_view(extra.error().where), "offset 3");
        BOOST_TEST(extra.error().what.empty());
    }
    const auto syntax = js::parse(R"({"a": tru})");
    if (BOOST_TEST(syntax.has_error())) {
        BOOST_TEST(syntax.error().code == boost::json::error::syntax);
        BOOST_TEST_EQ(std::string_view(syntax.error().where), "offset 6");  // where tru begins
    }
    const auto incomplete = js::parse(R"({"a": )");
    if (BOOST_TEST(incomplete.has_error())) {
        BOOST_TEST(incomplete.error().code == boost::json::error::incomplete);
        BOOST_TEST_EQ(std::string_view(incomplete.error().where), "offset 6");
    }
    BOOST_TEST(js::parse("").has_error());
    BOOST_TEST(js::parse("[1] ").has_value());
}

std::string nested(std::size_t depth) {
    return std::string(depth, '[') + std::string(depth, ']');
}

// The bound is exact: 256 levels are read and 257 are not, by default too, where Boost.JSON's own
// is 32; a smaller bound given is kept.
void the_depth_is_bounded() {
    BOOST_TEST_EQ(js::detail::max_parse_depth, 256U);
    BOOST_TEST(js::parse(nested(256)).has_value());
    BOOST_TEST_EQ(boost::json::parse_options().max_depth, 32U);
    BOOST_TEST(js::parse(nested(256), {}, boost::json::parse_options()).error().code ==
               boost::json::error::too_deep);
    BOOST_TEST(js::parse(nested(32), {}, boost::json::parse_options()).has_value());
    BOOST_TEST(js::parse(nested(257)).error().code == boost::json::error::too_deep);
    boost::json::parse_options asked;
    asked.max_depth = 2;
    BOOST_TEST(js::parse(nested(2), {}, asked).has_value());
    BOOST_TEST(js::parse(nested(3), {}, asked).error().code == boost::json::error::too_deep);
    asked.max_depth = 1000;
    BOOST_TEST(js::parse(nested(257), {}, asked).error().code == boost::json::error::too_deep);
}

void parse_allocates_through_the_given_allocator() {
    webcpp::test::counting_resource counted;
    {
        const auto parsed =
            js::parse(R"({"a": [1, 2, 3], "b": "a string long enough to leave SSO"})",
                      std::pmr::polymorphic_allocator<std::byte>(&counted));
        BOOST_TEST(parsed.has_value());
        BOOST_TEST(counted.allocations > 0);
    }
    BOOST_TEST_EQ(counted.live, 0U);
}

// Nothing reaches the default resource, a document that records numbers and one that fails
// included, and the value lives in storage that frees nothing piecemeal.
void nothing_reaches_the_default_resource() {
    webcpp::test::counting_resource elsewhere;
    webcpp::test::counting_resource counted;
    {
        const default_resource_guard guard(&elsewhere);
        const std::pmr::polymorphic_allocator<std::byte> allocator(&counted);
        {
            const auto parsed = js::parse(
                R"({"a key long enough to leave SSO": [9007199254740993.0, 1e400, "and more"],
                    "b": {"c": [[1.5, "a string long enough to leave SSO"]]}})",
                allocator);
            if (BOOST_TEST(parsed.has_value())) {
                BOOST_TEST(!parsed->exact_numbers.empty());
                BOOST_TEST(parsed->value.storage().is_deallocate_trivial());
                BOOST_TEST(parsed->exact_numbers.find("/a key long enough to leave SSO/1") !=
                           nullptr);
            }
            BOOST_TEST(
                js::parse(R"({"a": [9007199254740993.0, 1e400, tru]})", allocator).has_error());
            BOOST_TEST(js::parse(nested(300), allocator).has_error());
        }
    }
    BOOST_TEST_EQ(elsewhere.allocations, 0U);
    BOOST_TEST_EQ(elsewhere.deallocations, 0U);
    BOOST_TEST(counted.allocations > 0);
    BOOST_TEST_EQ(counted.live, 0U);
}

// An allocator that is not polymorphic holds everything, and gets every block back with the size
// and alignment it gave.
void any_allocator_holds_the_document() {
    webcpp::test::recording_resource recorded;
    webcpp::test::counting_resource elsewhere;
    {
        const default_resource_guard guard(&elsewhere);
        const auto parsed = js::parse<recording_traits>(
            R"({"a": [9007199254740993.0, "a string long enough to leave SSO"], "b": 1e400})",
            recording_traits::allocator_type(&recorded));
        if (BOOST_TEST(parsed.has_value())) {
            BOOST_TEST(parsed->exact_numbers.find("/a/0") != nullptr);
            BOOST_TEST(parsed->exact_numbers.find("/b") != nullptr);
            const boost::json::value& a = parsed->value.at("a");
            BOOST_TEST_EQ(a.at(1).as_string(), "a string long enough to leave SSO");
        }
    }
    BOOST_TEST(recorded.allocations > 0);
    BOOST_TEST_EQ(recorded.live_blocks(), 0U);
    BOOST_TEST_EQ(recorded.mismatched, 0U);
    BOOST_TEST_EQ(recorded.misaligned, 0U);
    BOOST_TEST_EQ(elsewhere.allocations, 0U);
    BOOST_TEST_EQ(elsewhere.deallocations, 0U);
}

// A long key over many recorded numbers: each pointer shares the key, so the memory stays linear
// in the text; a table of whole pointers would hold the key once per number, 4096 times 64 KiB
// here. Measured natively with clang: 8.2 times the text's size at the peak, of which the value
// alone, in its arena, with the parser's temporaries, is 4.2 times; Boost.JSON's own parse into
// a plain resource takes 1.9 times.
void the_table_is_linear_in_the_text() {
    const std::string key(std::size_t{64} * 1024, 'k');
    std::string text = "{\"" + key + "\": [";
    constexpr std::size_t numbers = 4096;
    for (std::size_t i = 0; i < numbers; ++i) {
        text += i == 0 ? "1e400" : ",1e400";
    }
    text += "], \"" + key + "x\": [";
    for (std::size_t i = 0; i < numbers; ++i) {
        text += i == 0 ? "0.5" : ",0.5";
    }
    text += "]}";
    webcpp::test::counting_resource counted;
    {
        const auto parsed = js::parse(text, std::pmr::polymorphic_allocator<std::byte>(&counted));
        if (BOOST_TEST(parsed.has_value())) {
            BOOST_TEST(parsed->exact_numbers.find("/" + key + "/0") != nullptr);
            BOOST_TEST(parsed->exact_numbers.find("/" + key + "/4095") != nullptr);
            BOOST_TEST(parsed->exact_numbers.find("/" + key + "/4096") == nullptr);
            BOOST_TEST(parsed->exact_numbers.find("/" + key + "x/0") == nullptr);
        }
    }
    BOOST_TEST_LE(counted.peak, 16 * text.size());
    BOOST_TEST_EQ(counted.live, 0U);
}

// The order a hashed table of this kind would keep: unseeded, the parent mixed in after the
// token's hash, so a key chosen to collide with others already in its bucket costs the bucket's
// whole chain. Reproduced here only to craft keys that collide under it; the table never consults
// a hash at all.
std::size_t legacy_hash_of(std::string_view token, std::size_t parent) {
    std::size_t seed = boost::hash<std::string_view>{}(token);
    boost::hash_combine(seed, parent);
    return seed;
}

// N keys of the form "a<i>" whose legacy hash collides in the same bucket of an
// `std::unordered_map` holding N entries.
std::vector<std::string> colliding_keys(std::size_t n) {
    std::unordered_map<std::size_t, std::size_t> probe;
    for (std::size_t i = 0; i < n; ++i) {
        probe.emplace(i, i);
    }
    const std::size_t buckets = probe.bucket_count();
    std::vector<std::string> keys;
    for (std::size_t i = 0; keys.size() < n; ++i) {
        std::string key = "a" + std::to_string(i);
        if (legacy_hash_of(key, 0) % buckets == 0) {
            keys.push_back(std::move(key));
        }
    }
    return keys;
}

std::string object_of(const std::vector<std::string>& keys) {
    std::string text = "{";
    for (std::size_t i = 0; i < keys.size(); ++i) {
        if (i != 0) {
            text += ',';
        }
        text += '"';
        text += keys[i];
        text += "\":1e400";
    }
    text += '}';
    return text;
}

// A comparator that counts every comparison it is asked for, on top of the table's real order;
// plugged in through `counting_traits::exact_compare`, which only a test names (`exact_handler`
// never does).
struct counting_step_less {
    mutable std::size_t comparisons = 0;
    using is_transparent = void;

    template <class A, class B>
    bool operator()(const A& a, const B& b) const noexcept {
        ++comparisons;
        return js::detail::exact_table_step_less{}(a, b);
    }
};

struct counting_traits : js::default_traits {
    using exact_compare = counting_step_less;
};

std::size_t comparisons_parsing_colliding_keys(std::size_t n) {
    const auto parsed = js::parse<counting_traits>(object_of(colliding_keys(n)));
    return parsed.has_value() ? parsed->exact_numbers.key_compare().comparisons : 0;
}

// A table that hashed a token and mixed in its parent afterwards let a document whose keys were
// chosen to collide cost quadratic time: 1.59 MB took 18.9 s to parse against 11 ms without
// collisions. The table orders its nodes instead of hashing them, at most O(log n) comparisons per
// key however the keys are chosen. 300 keys crafted to collide under the hashed table, and four
// times as many: a quadratic table would cost 16 times as many comparisons; measured here, the
// ordered table costs about 4 times as many (log(1200)/log(300) short of 4 x 4), so the bound
// below, at 8 times, both catches the regression and leaves a comfortable margin.
void the_table_is_not_quadratic_under_colliding_keys() {
    constexpr std::size_t n = 300;
    const std::size_t base = comparisons_parsing_colliding_keys(n);
    const std::size_t four_times = comparisons_parsing_colliding_keys(4 * n);
    if (!BOOST_TEST(base > 0)) {
        return;
    }
    BOOST_TEST_LT(four_times, 8 * base);
}

// Moving a parsed document moves its storage with its value: the value stays readable, and an
// assignment frees the document it replaces, with its storage, after its value.
void a_parsed_document_moves() {
    webcpp::test::counting_resource counted;
    const std::pmr::polymorphic_allocator<std::byte> allocator(&counted);
    {
        auto first =
            js::parse(R"({"a": [9007199254740993.0, "a string long enough to leave"]})", allocator);
        auto second =
            js::parse(R"({"b": {"c": [1e400, "another string long enough to leave"]}})", allocator);
        if (!BOOST_TEST(first.has_value() && second.has_value())) {
            return;
        }
        js::parsed_json moved(std::move(*first));
        const boost::json::value& a = moved.value.at("a");
        BOOST_TEST_EQ(a.at(1).as_string(), "a string long enough to leave");
        BOOST_TEST(moved.exact_numbers.find("/a/0") != nullptr);
        moved = std::move(*second);
        const boost::json::value& c = moved.value.at("b").at("c");
        BOOST_TEST_EQ(c.at(1).as_string(), "another string long enough to leave");
        BOOST_TEST(moved.exact_numbers.find("/b/c/0") != nullptr);
        BOOST_TEST(moved.exact_numbers.find("/a/0") == nullptr);
        js::parsed_json empty(allocator);
        BOOST_TEST(empty.value.is_null());
        BOOST_TEST(empty.exact_numbers.empty());
        moved = std::move(empty);
        BOOST_TEST(moved.value.is_null());
    }
    BOOST_TEST_EQ(counted.live, 0U);
}

}  // namespace

int main() {
    parse_keeps_what_a_double_loses();
    the_double_is_its_shortest_text();
    the_last_duplicate_wins_in_the_table_too();
    a_duplicate_with_the_same_double_drops_the_entry();
    pointers_are_rfc_6901();
    most_documents_record_nothing();
    the_options_given_are_kept_but_the_numbers();
    refusals_are_values();
    refusals_say_where();
    the_depth_is_bounded();
    parse_allocates_through_the_given_allocator();
    nothing_reaches_the_default_resource();
    any_allocator_holds_the_document();
    the_table_is_linear_in_the_text();
    the_table_is_not_quadratic_under_colliding_keys();
    a_parsed_document_moves();
    return boost::report_errors();
}
