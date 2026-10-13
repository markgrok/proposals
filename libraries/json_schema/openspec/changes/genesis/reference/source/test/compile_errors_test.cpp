// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/validator/validator.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>

#include <string>

namespace js = webcpp::json_schema;

namespace {

// load() never runs the meta-schema check, so every shape fault is the loader's own here.
const js::validator_for<js::draft2020_12> validator;

void refused(std::string_view schema, js::errc code, std::string_view where,
             std::string_view what = {}, const js::compile_options& options = {}) {
    const auto loaded = validator.load(boost::json::parse(schema), options);
    if (!BOOST_TEST(loaded.has_error())) {
        return;
    }
    BOOST_TEST(loaded.error().code == code);
    BOOST_TEST_EQ(std::string(loaded.error().where), std::string(where));
    if (!what.empty()) {
        BOOST_TEST_EQ(std::string(loaded.error().what), std::string(what));
    }
}

void load_errors() {
    refused(R"({"items": 1})", js::errc::not_a_schema, "/items");  // the child, refused by the walk
    refused(R"({"type": "integr"})", js::errc::invalid_keyword_value, "/type", "type");
    refused(R"({"$schema": "urn:other"})", js::errc::unknown_dialect, "/$schema", "urn:other");
    refused(R"({"maximum": 1e400})", js::errc::invalid_number, "/maximum", "maximum");
    refused(R"({"$ref": "#/x"})", js::errc::unresolvable_reference, "/$ref", "#/x");
    refused("{}", js::errc::unknown_dialect, "options", "http://json-schema.org/draft-07/schema#",
            {.default_dialect = js::dialect::draft7});
    refused(R"({"not": {"not": {"not": {"not": {}}}}})", js::errc::schema_too_deep,
            "/not/not/not/not", {}, {.max_schema_depth = 3});
    // The other codes a load returns.
    refused(R"({"$id": "urn:a#b"})", js::errc::invalid_uri, "/$id", "urn:a#b");
    refused(R"({"$defs": {"x": {"$anchor": "a"}, "y": {"$anchor": "a"}}})",
            js::errc::duplicate_anchor, "/$defs/y/$anchor", "a");
    refused(R"({"$dynamicRef": "#meta"})", js::errc::unresolvable_reference, "/$dynamicRef",
            "#meta");
    refused(R"({"items": {"$id": "urn:b"}})", js::errc::unresolvable_reference, "/items/$id",
            "urn:b");
    refused("{}", js::errc::invalid_keyword_value, "options", "format_assertion",
            {.format_assertion = true});
}

}  // namespace

int main() {
    load_errors();
    return boost::report_errors();
}
