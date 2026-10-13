// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The regex policy over webcpp::regex: ECMAScript's semantics, unanchored, on UTF-8; refusals as
// values (a bad pattern, a leading (?i), a pattern nested past webcpp::regex::max_nesting or with
// more alternatives than its max_alternatives); the step budget reaching the regex; a runaway
// search a limit; the schema's allocator holding the regex and the validation's the search, a
// short subject inline, nothing in the default resource, and a validation's allocator that is not
// polymorphic.

#include <webcpp/json_schema/regex/webcpp_regex.hpp>

#include <webcpp/regex/error.hpp>
#include <webcpp/regex/options.hpp>

#include "counting_resource.hpp"
#include "recording_resource.hpp"

#include <boost/core/lightweight_test.hpp>

#include <cstddef>
#include <memory_resource>
#include <string>

namespace js = webcpp::json_schema;
using alloc = std::pmr::polymorphic_allocator<std::byte>;

static_assert(js::regex_policy<js::webcpp_regex, alloc, alloc>);
static_assert(
    js::regex_policy<js::webcpp_regex, alloc, webcpp::test::recording_allocator<std::byte>>);

namespace {

js::match_result search(std::string_view pattern, std::string_view text) {
    std::pmr::monotonic_buffer_resource schema;
    auto re = js::webcpp_regex::compile(pattern, alloc(&schema), 0);
    if (!BOOST_TEST(re.has_value())) {
        return js::match_result::limit;
    }
    std::pmr::monotonic_buffer_resource validation;
    return js::webcpp_regex::search(*re, text, alloc(&validation));
}

void ecmascript_semantics() {
    BOOST_TEST(search("^[a-z]+$", "abc") == js::match_result::found);
    BOOST_TEST(search("^[a-z]+$", "ab1") == js::match_result::not_found);
    BOOST_TEST(search("b", "abc") == js::match_result::found);  // unanchored
    BOOST_TEST(search("^abc$", "abc\n") == js::match_result::not_found);
    BOOST_TEST(search("^.$", "\xF0\x9F\x90\xB2") == js::match_result::found);
    BOOST_TEST(search("^\\p{Letter}+$", "\xC3\x89\x63ole") == js::match_result::found);
    BOOST_TEST(search("(?<=\\$\\d*)x", "$12x") == js::match_result::found);
}

void refusals_are_values() {
    std::pmr::monotonic_buffer_resource schema;
    BOOST_TEST(!js::webcpp_regex::compile("(", alloc(&schema), 0));
    BOOST_TEST(!js::webcpp_regex::compile("a{,5}", alloc(&schema), 0));
    BOOST_TEST(!js::webcpp_regex::compile("(?i)a", alloc(&schema), 0));
    const std::string nested = std::string(10000, '(') + "a" + std::string(10000, ')');
    const auto deep = js::webcpp_regex::compile(nested, alloc(&schema), 0);
    BOOST_TEST(deep.has_error() && deep.error() == webcpp::regex::error::too_deep);
    std::string alternatives = "a";
    for (std::size_t i = 1; i < webcpp::regex::max_alternatives; ++i) {
        alternatives += "|a";
    }
    BOOST_TEST(js::webcpp_regex::compile(alternatives, alloc(&schema), 0).has_value());
    const auto wide = js::webcpp_regex::compile(alternatives + "|a", alloc(&schema), 0);
    BOOST_TEST(wide.has_error() && wide.error() == webcpp::regex::error::too_complex);
}

void the_step_budget_reaches_the_regex() {
    std::pmr::monotonic_buffer_resource schema;
    const auto budgeted = js::webcpp_regex::compile("(a*)*b", alloc(&schema), 1'000);
    if (!BOOST_TEST(budgeted.has_value())) {
        return;
    }
    BOOST_TEST_EQ(budgeted->compile_options().step_budget, 1'000U);
    std::pmr::monotonic_buffer_resource validation;
    BOOST_TEST(js::webcpp_regex::search(*budgeted, std::string(40, 'a'), alloc(&validation)) ==
               js::match_result::limit);
}

void a_runaway_search_is_a_limit() {
    BOOST_TEST(
        search("^(\\w+\\s?)*$", "A long sentence of words that ends with something refused!") ==
        js::match_result::limit);
    BOOST_TEST(search("(x+x+)+y", std::string(30, 'x')) == js::match_result::limit);
}

void storage_goes_where_it_is_told() {
    webcpp::test::counting_resource schema;
    webcpp::test::counting_resource validation;
    webcpp::test::counting_resource elsewhere;
    std::pmr::memory_resource* const previous = std::pmr::set_default_resource(&elsewhere);
    auto re = js::webcpp_regex::compile("^[a-z]+$", alloc(&schema), 0);
    if (!BOOST_TEST(re.has_value())) {
        std::pmr::set_default_resource(previous);
        return;
    }
    BOOST_TEST(schema.allocations > 0);
    BOOST_TEST(re->get_allocator().resource() == &schema);
    const std::string long_subject(100000, 'a');
    BOOST_TEST(js::webcpp_regex::search(*re, long_subject, alloc(&validation)) ==
               js::match_result::found);
    BOOST_TEST(validation.allocations > 0);  // past the inline buffer, into the given resource
    BOOST_TEST_EQ(validation.live, 0U);
    const std::size_t before = validation.allocations;
    BOOST_TEST(js::webcpp_regex::search(*re, "abc", alloc(&validation)) == js::match_result::found);
    BOOST_TEST_EQ(validation.allocations, before);  // a short subject stays inline
    std::pmr::set_default_resource(previous);
    BOOST_TEST_EQ(elsewhere.allocations, 0U);
}

// The validation's allocator need not be polymorphic: the search's state past the inline buffer
// reaches any allocator, and is all given back, at the size and alignment it was taken with.
void any_validation_allocator_holds_the_search() {
    std::pmr::monotonic_buffer_resource schema;
    const auto re = js::webcpp_regex::compile("^[a-z]+$", alloc(&schema), 0);
    if (!BOOST_TEST(re.has_value())) {
        return;
    }
    webcpp::test::recording_resource validation;
    const webcpp::test::recording_allocator<std::byte> allocator(&validation);
    BOOST_TEST(js::webcpp_regex::search(*re, std::string(100000, 'a'), allocator) ==
               js::match_result::found);
    BOOST_TEST(validation.allocations > 0);
    BOOST_TEST_EQ(validation.live_blocks(), 0U);
    BOOST_TEST_EQ(validation.mismatched, 0U);
    BOOST_TEST_EQ(validation.misaligned, 0U);
    const std::size_t before = validation.allocations;
    BOOST_TEST(js::webcpp_regex::search(*re, "ab1", allocator) == js::match_result::not_found);
    BOOST_TEST_EQ(validation.allocations, before);
}

}  // namespace

int main() {
    ecmascript_semantics();
    refusals_are_values();
    the_step_budget_reaches_the_regex();
    a_runaway_search_is_a_limit();
    storage_goes_where_it_is_told();
    any_validation_allocator_holds_the_search();
    return boost::report_errors();
}
