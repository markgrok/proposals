// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/detail/compiler/compile.hpp>
#include <webcpp/json_schema/detail/compiler/explain.hpp>
#include <webcpp/json_schema/validator/validator.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace js = webcpp::json_schema;

namespace {

const js::validator_for<js::draft2020_12> validator;

std::string plan(std::string_view schema) {
    const auto document = validator.load(boost::json::parse(schema));
    const auto compiled = js::detail::compile_plan(*document, {});
    if (!compiled) {
        return std::string(compiled.error().code.message()) + " " + std::string(compiled.error().where);
    }
    std::pmr::monotonic_buffer_resource scratch;
    return std::string(js::detail::explain(*document, *compiled, scratch));
}

void assertions_are_guarded_by_type() {
    BOOST_TEST_EQ(plan(R"({"type": "string", "maxLength": 3})"),
                  "block 0 urn:webcpp:root#\n"
                  "  0 assert_type string ; urn:webcpp:root#/type\n"
                  "  1 type_guard string 1 ; urn:webcpp:root#/maxLength\n"
                  "  2 assert_length max 3 ; urn:webcpp:root#/maxLength\n"
                  "  3 ret\n");
}

void blocks_follow_a_breadth_first_walk() {
    BOOST_TEST_EQ(plan(R"({"properties": {"a": {"minLength": 1}}, "required": ["a"], "allOf": [true, {"maximum": 2}]})"),
                  "block 0 urn:webcpp:root#\n"
                  "  0 type_guard object 1 ; urn:webcpp:root#/properties\n"
                  "  1 loop_properties names 1 patterns 0 additional none ; urn:webcpp:root#/properties\n"
                  "  2 type_guard object 1 ; urn:webcpp:root#/required\n"
                  "  3 assert_required a ; urn:webcpp:root#/required\n"
                  "  4 all_of 2 3 ; urn:webcpp:root#/allOf\n"
                  "  5 ret\n"
                  "block 1 urn:webcpp:root#/properties/a\n"
                  "  0 type_guard string 1 ; urn:webcpp:root#/properties/a/minLength\n"
                  "  1 assert_length min 1 ; urn:webcpp:root#/properties/a/minLength\n"
                  "  2 ret\n"
                  "block 2 urn:webcpp:root#/allOf/0\n"
                  "  0 ret\n"
                  "block 3 urn:webcpp:root#/allOf/1\n"
                  "  0 type_guard number 1 ; urn:webcpp:root#/allOf/1/maximum\n"
                  "  1 assert_maximum 2 ; urn:webcpp:root#/allOf/1/maximum\n"
                  "  2 ret\n");
}

void a_bad_pattern_is_refused_by_place() {
    BOOST_TEST_EQ(plan(R"({"properties": {"a": {"pattern": "("}}})"),
                  "the pattern is not a valid regular expression /properties/a/pattern");
    const auto document = validator.load(boost::json::parse(R"({"pattern": "("})"));
    BOOST_TEST_EQ(std::string(js::detail::compile_plan(*document, {}).error().what), "(");
}

}  // namespace

int main() {
    assertions_are_guarded_by_type();
    blocks_follow_a_breadth_first_walk();
    a_bad_pattern_is_refused_by_place();
    return boost::report_errors();
}
