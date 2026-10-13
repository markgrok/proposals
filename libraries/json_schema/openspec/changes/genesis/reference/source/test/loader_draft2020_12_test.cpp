// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/detail/numbers/compare.hpp>
#include <webcpp/json_schema/detail/numbers/decompose.hpp>
#include <webcpp/json_schema/validator/validator.hpp>

#include "counting_resource.hpp"
#include "recording_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/decimal/decimal128_t.hpp>
#include <boost/json/monotonic_resource.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/value.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace js = webcpp::json_schema;
namespace ir = js::ir;
using k = ir::keyword_kind;

namespace {

const js::validator_for<js::draft2020_12> validator;

js::result<js::ir::document> load(std::string_view text) {
    return validator.load(boost::json::parse(text));
}

const ir::keyword* find(const js::ir::document& d, k kind) {
    return d.find(d.root(), kind);
}

// The JSON Pointer of a node, spelled.
std::string pointer_of(const js::ir::document& d, ir::node_id node) {
    std::string out;
    d.write_pointer(d.at(node).pointer, out);
    return out;
}

void boolean_schemas() {
    const auto yes = load("true");
    BOOST_TEST(yes->at(yes->root()).kind == ir::node_kind::always_valid);
    const auto no = load("false");
    BOOST_TEST(no->at(no->root()).kind == ir::node_kind::never_valid);
    BOOST_TEST(load("3").error().code == js::errc::not_a_schema);
}

void applicators_keep_source_order() {
    const auto d = load(
        R"({"prefixItems": [true, false], "items": false, "properties": {"b": true, "a": {}}})");
    if (!BOOST_TEST(d.has_value())) {
        return;
    }
    const auto prefix = std::get<ir::subschema_list>(find(*d, k::prefix_items)->data);
    BOOST_TEST_EQ(d->nodes(prefix.nodes).size(), 2U);
    const auto entries =
        d->entries(std::get<ir::named_subschemas>(find(*d, k::properties)->data).entries);
    BOOST_TEST_EQ(d->string(entries[0].name), "b");
    BOOST_TEST_EQ(pointer_of(*d, entries[1].schema), "/properties/a");
    BOOST_TEST(find(*d, k::items) != nullptr);
}

void dialect_fields_are_explicit() {
    const auto d = load(R"({"type": ["integer", "null"], "contains": true, "minContains": 2.0,
                            "format": "email", "exclusiveMaximum": 3, "dependentRequired": {"a": ["b"]}})");
    if (!BOOST_TEST(d.has_value())) {
        return;
    }
    BOOST_TEST(std::get<ir::types>(find(*d, k::type)->data).integer_by_value);
    BOOST_TEST(std::get<ir::contains_payload>(find(*d, k::contains)->data).annotates_items);
    BOOST_TEST_EQ(std::get<ir::count>(find(*d, k::min_contains)->data).value, 2U);
    BOOST_TEST(!std::get<ir::format_payload>(find(*d, k::format)->data).assertion);
    BOOST_TEST(find(*d, k::exclusive_maximum) != nullptr);
    BOOST_TEST(find(*d, k::dependent_required) != nullptr);
}

void sources_are_views_of_the_document() {
    const auto d = load(R"({"enum": [{"$id": "urn:data"}, 2], "title": "t"})");
    BOOST_TEST_EQ(d->resources().size(), 1U);  // an $id inside enum is data
    BOOST_TEST_EQ(d->value(find(*d, k::title)->source), "t");
}

void anchors_and_unknown_keywords() {
    const auto d = load(R"({"$anchor": "a", "$dynamicAnchor": "d", "x-note": {"$id": "urn:no"},
                            "$comment": "c", "definitions": {}})");
    if (!BOOST_TEST(d.has_value())) {
        return;
    }
    BOOST_TEST_EQ(d->anchors().size(), 3U);    // a; d dynamic; d plain
    BOOST_TEST_EQ(d->resources().size(), 1U);  // the $id inside x-note registers nothing
    BOOST_TEST(find(*d, k::unknown) != nullptr);
    BOOST_TEST(find(*d, k::comment) != nullptr);
}

void schema_uri_spellings() {
    BOOST_TEST(load(R"({"$schema": "https://json-schema.org/draft/2020-12/schema#"})").has_value());
    BOOST_TEST(load(R"({"$schema": "https://json-schema.org/draft/2020-12/schema"})").has_value());
    const auto draft7 = load(R"({"$schema": "http://json-schema.org/draft-07/schema#"})");
    BOOST_TEST(draft7.error().code == js::errc::unknown_dialect);
    BOOST_TEST_EQ(std::string(draft7.error().what), "http://json-schema.org/draft-07/schema#");
    const auto https7 = load(R"({"$schema": "https://json-schema.org/draft-07/schema#"})");
    BOOST_TEST(https7.error().code == js::errc::unknown_dialect);
    BOOST_TEST_EQ(std::string(https7.error().what), "https://json-schema.org/draft-07/schema#");
}

void shapes_are_refused_by_place() {
    const auto d = load(R"({"properties": {"a": {"minLength": -1}}})");
    BOOST_TEST(d.error().code == js::errc::invalid_keyword_value);
    BOOST_TEST_EQ(std::string(d.error().where), "/properties/a/minLength");
    BOOST_TEST_EQ(std::string(d.error().what), "minLength");
    BOOST_TEST(load(R"({"multipleOf": 0})").error().code == js::errc::invalid_keyword_value);
    BOOST_TEST(load(R"({"required": ["a", "a"]})").error().code == js::errc::invalid_keyword_value);
    BOOST_TEST(load(R"({"allOf": []})").error().code == js::errc::invalid_keyword_value);
}

void exact_numbers_reach_the_ir() {
    const auto parsed = js::parse(R"({"maximum": 9007199254740993.0})");
    const auto d = validator.load(*parsed);
    const auto& n = d->number(std::get<ir::bound>(find(*d, k::maximum)->data).value);
    BOOST_TEST(js::detail::compare(js::detail::decompose(n),
                                   js::detail::decompose_int64(9007199254740993)) == 0);
    BOOST_TEST(load(R"({"maximum": 1e400})").error().code == js::errc::invalid_number);
}

// The checks below cover every reader, the options, the resource keywords, the allocator, the
// depth and the order of refusals.

static_assert(std::is_same_v<js::validator, js::validator_for<js::draft2020_12>>);
static_assert(js::validator::dialects.size() == 1 &&
              js::validator::dialects[0] == js::dialect::draft2020_12);
static_assert(std::is_final_v<js::validator>);

struct recording_traits : js::default_traits {
    using allocator_type = webcpp::test::recording_allocator<std::byte>;
};

struct wide_traits : js::default_traits {
    using number_type = boost::decimal::decimal128_t;
};

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

// A load's refusal: its code, where and what.
void refused(std::string_view schema, js::errc code, std::string_view where, std::string_view what,
             const js::compile_options& options = {}) {
    const auto loaded = validator.load(boost::json::parse(schema), options);
    if (!BOOST_TEST(loaded.has_error())) {
        return;
    }
    BOOST_TEST(loaded.error().code == code);
    BOOST_TEST_EQ(std::string(loaded.error().where), std::string(where));
    BOOST_TEST_EQ(std::string(loaded.error().what), std::string(what));
}

// Every keyword of the table, in one schema: its kind, its payload, its name and its source as
// written, and a document that passes its check.
void every_keyword_of_the_table() {
    const boost::json::value schema = boost::json::parse(R"({
        "$defs": {"a": true},
        "$comment": "c",
        "allOf": [true],
        "anyOf": [true],
        "oneOf": [true],
        "not": false,
        "if": true,
        "then": true,
        "else": false,
        "dependentSchemas": {"a": true},
        "prefixItems": [true],
        "items": true,
        "contains": true,
        "properties": {"a": true},
        "patternProperties": {"^a": true},
        "additionalProperties": false,
        "propertyNames": true,
        "unevaluatedItems": false,
        "unevaluatedProperties": false,
        "type": "object",
        "enum": [1, "a"],
        "const": {"a": [1]},
        "multipleOf": 0.5,
        "maximum": 10,
        "exclusiveMaximum": 11,
        "minimum": -1,
        "exclusiveMinimum": -2,
        "maxLength": 5,
        "minLength": 0,
        "pattern": "^a",
        "maxItems": 3,
        "minItems": 1,
        "uniqueItems": true,
        "maxContains": 2,
        "minContains": 1,
        "maxProperties": 4,
        "minProperties": 1,
        "required": ["a"],
        "dependentRequired": {"a": ["b"]},
        "title": "t",
        "description": "d",
        "default": null,
        "deprecated": false,
        "readOnly": true,
        "writeOnly": false,
        "examples": [1],
        "format": "date",
        "contentEncoding": "base64",
        "contentMediaType": "text/plain",
        "contentSchema": true
    })");
    const auto d = validator.load(schema);
    if (!BOOST_TEST(d.has_value())) {
        return;
    }
    BOOST_TEST(d->check().has_value());
    const auto keywords = d->keywords(d->root());
    BOOST_TEST_EQ(keywords.size(), schema.as_object().size());
    const auto table = js::detail::dialect_machinery<js::draft2020_12>::keywords();
    for (const ir::keyword& keyword : keywords) {
        const std::string_view name = d->string(keyword.name);
        const auto* rule = js::detail::rule_named(table, name);
        if (!BOOST_TEST(rule != nullptr)) {
            continue;
        }
        BOOST_TEST(keyword.kind == rule->kind);
        BOOST_TEST_EQ(keyword.data.index(), ir::payload_index(keyword.kind));
        BOOST_TEST(d->value(keyword.source) == schema.as_object().at(name));
    }
    const auto& resource = d->resources()[0];
    BOOST_TEST_EQ(d->string(resource.uri), "urn:webcpp:root");
    BOOST_TEST(resource.source_dialect == js::dialect::draft2020_12);
    namespace v = ir::vocabulary_bit;
    for (const std::uint16_t bit : {
             v::core,
             v::applicator,
             v::unevaluated,
             v::validation,
             v::meta_data,
             v::format_annotation,
             v::content,
         }) {
        BOOST_TEST((resource.vocabularies & bit) != 0);
    }
    BOOST_TEST((resource.vocabularies & v::format_assertion) == 0);
    BOOST_TEST_EQ(table.size(), 52U);
}

// Each reader refuses a value of the wrong shape, at the keyword's pointer, naming it.
void every_reader_refuses_its_wrong_shapes() {
    constexpr std::array<std::string_view, 36> cases{
        R"({"allOf": {}})",
        R"({"prefixItems": []})",
        R"({"$defs": []})",
        R"({"patternProperties": true})",
        R"({"type": []})",
        R"({"type": ["string", "string"]})",
        R"({"type": 1})",
        R"({"type": ["string", 1]})",
        R"({"uniqueItems": 1})",
        R"({"enum": {}})",
        R"({"maximum": "1"})",
        R"({"multipleOf": -2})",
        R"({"multipleOf": 0.0})",
        R"({"minLength": 2.5})",
        R"({"minLength": -1.0})",
        R"({"minLength": "1"})",
        R"({"maxItems": true})",
        R"({"pattern": 1})",
        R"({"contentEncoding": null})",
        R"({"required": "a"})",
        R"({"required": [1]})",
        R"({"dependentRequired": []})",
        R"({"dependentRequired": {"a": "b"}})",
        R"({"dependentRequired": {"a": ["b", "b"]}})",
        R"({"format": 1})",
        R"({"$ref": 1})",
        R"({"$dynamicRef": {}})",
        R"({"$comment": 1})",
        R"({"title": 1})",
        R"({"description": []})",
        R"({"deprecated": "yes"})",
        R"({"readOnly": 1})",
        R"({"writeOnly": null})",
        R"({"examples": {}})",
        R"({"$schema": 1})",
        R"({"$vocabulary": {"urn:a": 1}})",
    };
    for (const std::string_view schema : cases) {
        // the one keyword of the schema is the one refused
        const std::string name(boost::json::parse(schema).as_object().begin()->key());
        refused(schema, js::errc::invalid_keyword_value, "/" + name, name);
    }
    // default and const take any value
    BOOST_TEST(load(R"({"default": {"x": [1]}, "const": null})").has_value());
}

// A count is a non-negative integer, written with a zero fraction too; one past std::uint64_t
// is its largest value.
void counts() {
    const auto count_of = [](std::string_view schema) -> std::uint64_t {
        const auto d = load(schema);
        if (!BOOST_TEST(d.has_value())) {
            return 0;
        }
        return std::get<ir::count>(find(*d, k::min_length)->data).value;
    };
    BOOST_TEST_EQ(count_of(R"({"minLength": 0})"), 0U);
    BOOST_TEST_EQ(count_of(R"({"minLength": -0.0})"), 0U);
    BOOST_TEST_EQ(count_of(R"({"minLength": 1e2})"), 100U);
    BOOST_TEST_EQ(count_of(R"({"minLength": 18446744073709551615})"),
                  std::numeric_limits<std::uint64_t>::max());
    BOOST_TEST_EQ(count_of(R"({"minLength": 1e30})"), std::numeric_limits<std::uint64_t>::max());
    BOOST_TEST(load(R"({"minLength": 1e400})").error().code == js::errc::invalid_number);
    // Read from its text, a fraction too small for a double is still a fraction, where the number
    // type holds it.
    constexpr std::string_view fraction = R"({"minLength": 2.0000000000000000001})";
    const js::basic_validator<wide_traits, js::draft2020_12> wide;
    const auto exact = wide.load(*js::parse<wide_traits>(fraction));
    BOOST_TEST(exact.error().code == js::errc::invalid_keyword_value);
    BOOST_TEST(load(fraction).has_value());
    BOOST_TEST(validator.load(*js::parse(fraction)).has_value());  // decimal64 rounds it to 2
}

// A bound keeps an integer that fits as one, and any other number as a Number; the exact number
// of the text counts only while the double it was recorded with is still there.
void bounds_keep_their_kind() {
    const auto number_of = [](const js::result<js::ir::document>& d) {
        return d->number(std::get<ir::bound>(find(*d, k::maximum)->data).value);
    };
    BOOST_TEST_EQ(number_of(load(R"({"maximum": 3})")).index(), 0U);
    BOOST_TEST_EQ(number_of(load(R"({"maximum": 18446744073709551615})")).index(), 1U);
    BOOST_TEST_EQ(number_of(load(R"({"maximum": 2.0})")).index(), 0U);
    BOOST_TEST_EQ(number_of(load(R"({"maximum": 1.5})")).index(), 2U);
    auto parsed = js::parse(R"({"maximum": 9007199254740993.0, "minimum": 0.1})");
    const auto exact = validator.load(*parsed);
    BOOST_TEST_EQ(number_of(exact).index(), 0U);
    BOOST_TEST(exact->check().has_value());
    parsed->value.as_object()["maximum"] = 1.5;
    const auto changed = validator.load(*parsed);
    const auto n = number_of(changed);
    const auto one_and_a_half = js::detail::decompose_double(1.5);
    BOOST_TEST(one_and_a_half.has_value());
    if (one_and_a_half.has_value()) {
        BOOST_TEST(js::detail::compare(js::detail::decompose(n), *one_and_a_half) == 0);
    }
    BOOST_TEST(js::parse(R"({"maximum": 1e400})").has_value());
    BOOST_TEST(validator.load(*js::parse(R"({"maximum": 1e400})")).error().code ==
               js::errc::invalid_number);
}

// $anchor and $dynamicAnchor: their syntax, and a name given twice in a resource.
void anchors() {
    refused(R"({"$anchor": "1a"})", js::errc::invalid_keyword_value, "/$anchor", "$anchor");
    refused(R"({"$dynamicAnchor": 1})", js::errc::invalid_keyword_value, "/$dynamicAnchor",
            "$dynamicAnchor");
    refused(R"({"$anchor": "a", "items": {"$anchor": "a"}})", js::errc::duplicate_anchor,
            "/items/$anchor", "a");
    refused(R"({"$dynamicAnchor": "a", "items": {"$anchor": "a"}})", js::errc::duplicate_anchor,
            "/items/$anchor", "a");
    const auto same =
        load(R"({"$anchor": "a", "$dynamicAnchor": "a", "items": {"$anchor": "b-1._"}})");
    if (!BOOST_TEST(same.has_value())) {
        return;
    }
    BOOST_TEST_EQ(same->anchors().size(), 3U);  // a plain, a dynamic, b-1._ plain
    BOOST_TEST(same->check().has_value());
    const auto items = std::get<ir::subschema>(find(*same, k::items)->data).schema;
    BOOST_TEST(same->anchors()[2].target == items);
}

// The root's $id names its resource; a relative or nested one is refused until references exist.
void ids() {
    const auto absolute = load(R"({"$id": "https://example.com/root.json#"})");
    BOOST_TEST_EQ(absolute->string(absolute->resources()[0].uri), "https://example.com/root.json");
    BOOST_TEST(absolute->check().has_value());
    const auto empty = load(R"({"$id": "#"})");
    BOOST_TEST_EQ(empty->string(empty->resources()[0].uri), "urn:webcpp:root");
    refused(R"({"$id": "https://example.com/a#b"})", js::errc::invalid_uri, "/$id",
            "https://example.com/a#b");
    refused(R"({"$id": "root.json"})", js::errc::unresolvable_reference, "/$id", "root.json");
    refused(R"({"$id": 1})", js::errc::invalid_keyword_value, "/$id", "$id");
    refused(R"({"items": {"$id": "urn:inner"}})", js::errc::unresolvable_reference, "/items/$id",
            "urn:inner");
}

// Options, and the name of the one a load refuses among them.
struct option_case {
    js::compile_options options;
    std::string_view name;
};

// Options a load cannot honour are refused by name; those it honours are not.
void options() {
    js::registry registry;
    const auto always = [](std::string_view) noexcept { return true; };
    const std::array<js::format_entry, 1> formats{js::format_entry{.name = "x", .check = always}};
    const auto resolver = [](std::string_view, const boost::json::storage_ptr&) {
        return boost::system::result<boost::json::value>(boost::json::value{});
    };
    const std::array<option_case, 8> unread{
        option_case{.options = {.registry = &registry}, .name = "registry"},
        option_case{.options = {.resolver = resolver}, .name = "resolver"},
        option_case{.options = {.base_uri = "https://example.com/"}, .name = "base_uri"},
        option_case{.options = {.format_assertion = true}, .name = "format_assertion"},
        option_case{.options = {.formats = formats}, .name = "formats"},
        option_case{.options = {.content_assertion = true}, .name = "content_assertion"},
        option_case{.options = {.unknown = js::unknown_keywords::reject}, .name = "unknown"},
        option_case{.options = {.unknown = js::unknown_keywords::ignore}, .name = "unknown"},
    };
    for (const option_case& c : unread) {
        refused("{}", js::errc::invalid_keyword_value, "options", c.name, c.options);
    }
    refused("{}", js::errc::unknown_dialect, "options", "openapi3_0",
            {.default_dialect = js::dialect::openapi3_0});
    // a default dialect outside the set is refused whatever $schema says
    refused(R"({"$schema": "https://json-schema.org/draft/2020-12/schema"})",
            js::errc::unknown_dialect, "options", "https://json-schema.org/draft/2019-09/schema",
            {.default_dialect = js::dialect::draft2019_09});
    for (const js::compile_options& honoured : {
             js::compile_options{.default_dialect = js::dialect::draft2020_12},
             js::compile_options{.regex_step_budget = 10},
             js::compile_options{.unknown = js::unknown_keywords::annotate},
             js::compile_options{.compatibility_keywords = false},
             js::compile_options{.validate_schema = false},
             js::compile_options{.max_schema_depth = 0},
         }) {
        BOOST_TEST(validator.load(boost::json::parse("{}"), honoured).has_value());
    }
}

// A keyword name, a member name or a string that is not UTF-8 is refused: no string of the
// document may hold one (invariant 8).
void text_that_is_not_utf8() {
    const std::string bad("\xC3\x28", 2);
    const boost::json::value keyword_name = boost::json::object{{bad, 1}};
    const auto name = validator.load(keyword_name);
    BOOST_TEST(name.error().code == js::errc::invalid_keyword_value);
    BOOST_TEST_EQ(std::string(name.error().where), "");
    const boost::json::value member =
        boost::json::object{{"properties", boost::json::object{{bad, true}}}};
    const auto in_map = validator.load(member);
    BOOST_TEST(in_map.error().code == js::errc::invalid_keyword_value);
    BOOST_TEST_EQ(std::string(in_map.error().where), "/properties");
    const boost::json::value text = boost::json::object{{"required", boost::json::array{bad}}};
    BOOST_TEST_EQ(std::string(validator.load(text).error().what), "required");
    const boost::json::value inert =
        boost::json::object{{"const", bad}, {"x-a", boost::json::array{bad}}};
    BOOST_TEST(validator.load(inert).has_value());  // values are data, never strings of the IR
}

// A node's keywords are read before its subschemas are walked, and its subschemas are walked in
// document order: the first refusal is the first of that order.
void refusals_come_in_document_order() {
    refused(R"({"not": 1, "items": 2})", js::errc::not_a_schema, "/not", "");
    refused(R"({"allOf": [{"not": 1}], "items": 2})", js::errc::not_a_schema, "/allOf/0/not", "");
    refused(R"({"items": 1, "minLength": -1})", js::errc::invalid_keyword_value, "/minLength",
            "minLength");
    refused(R"({"properties": {"a/b~": 1}})", js::errc::not_a_schema, "/properties/a~1b~0", "");
}

// A subschema of an unknown keyword, enum or const is not walked: no refusal, no node.
void unknown_keywords_hold_data() {
    const auto d = load(R"({"x": {"type": 1, "$id": "rel"}, "definitions": {"a": {"minLength": -1}},
                            "dependencies": {"a": 1}})");
    if (!BOOST_TEST(d.has_value())) {
        return;
    }
    BOOST_TEST_EQ(d->size(), 1U);
    std::size_t unknown = 0;
    for (const ir::keyword& keyword : d->keywords(d->root())) {
        unknown += keyword.kind == k::unknown ? 1 : 0;
        BOOST_TEST(std::get<ir::constant>(keyword.data).value == keyword.source);
    }
    BOOST_TEST_EQ(unknown, 3U);
    BOOST_TEST(d->check().has_value());
}

// A schema as deep as the default limit loads, with a node for each level and no recursion; one
// level deeper is refused.
void nested_to_the_limit() {
    boost::json::monotonic_resource storage;
    const auto nested = [&](std::size_t levels) {
        boost::json::value v = boost::json::object(&storage);
        for (std::size_t i = 0; i < levels; ++i) {
            boost::json::object o(&storage);
            o["if"] = std::move(v);
            v = std::move(o);
        }
        return v;
    };
    const auto at_limit = validator.load(nested(js::default_max_schema_depth));
    if (BOOST_TEST(at_limit.has_value())) {
        BOOST_TEST_EQ(at_limit->size(), js::default_max_schema_depth + 1);
        BOOST_TEST(at_limit->check().has_value());
    }
    const auto past = validator.load(nested(js::default_max_schema_depth + 1));
    BOOST_TEST(past.error().code == js::errc::schema_too_deep);
    BOOST_TEST(std::string_view(past.error().where).starts_with("/if/if/if"));
    BOOST_TEST(std::string_view(past.error().where).ends_with("..."));
}

// Everything a load allocates, a refused one too, goes through its allocator, the adapter of a
// non-pmr one included, and is given back with the document.
void allocations_go_through_the_allocator() {
    const boost::json::value schema = boost::json::parse(R"({
        "properties": {"a": {"type": "string", "enum": ["x", "y"]}},
        "required": ["a"],
        "dependentRequired": {"a": ["b"]},
        "$anchor": "top",
        "maximum": 1.5
    })");
    const boost::json::value wrong =
        boost::json::parse(R"({"properties": {"a": {"minLength": -1}}})");
    webcpp::test::counting_resource elsewhere;
    const default_resource_guard guard(&elsewhere);
    webcpp::test::counting_resource used;
    {
        const auto d = validator.load(schema, js::compile_options(),
                                      std::pmr::polymorphic_allocator<std::byte>(&used));
        BOOST_TEST(d.has_value());
        BOOST_TEST(validator
                       .load(wrong, js::compile_options(),
                             std::pmr::polymorphic_allocator<std::byte>(&used))
                       .has_error());
    }
    BOOST_TEST_EQ(elsewhere.allocations, 0U);
    BOOST_TEST_GT(used.allocations, 0U);
    BOOST_TEST_EQ(used.live, 0U);

    webcpp::test::recording_resource recorded;
    const js::basic_validator<recording_traits, js::draft2020_12> recording;
    {
        const auto d = recording.load(schema, js::basic_compile_options<recording_traits>(),
                                      webcpp::test::recording_allocator<std::byte>(&recorded));
        BOOST_TEST(d.has_value() && d->check().has_value());
    }
    BOOST_TEST_EQ(elsewhere.allocations, 0U);
    BOOST_TEST_GT(recorded.allocations, 0U);
    BOOST_TEST_EQ(recorded.live_blocks(), 0U);
    BOOST_TEST_EQ(recorded.mismatched, 0U);
}

// The most bytes a load of `schema` holds at once through its allocator.
template <class Schema>
std::size_t peak_of(const Schema& schema) {
    webcpp::test::counting_resource counted;
    {
        const auto d = validator.load(schema, js::compile_options(),
                                      std::pmr::polymorphic_allocator<std::byte>(&counted));
        BOOST_TEST(d.has_value());
    }
    return counted.peak;
}

// What a load holds is linear in its input, whatever its keys' lengths and its depth, since a
// pointer is held as one step per token and never whole. The constant is measured, not derived:
// the most bytes per byte of text come from the smallest items, a list of empty subschemas (`{}`,
// 3 bytes of text, about 83 bytes held) and an enum of one-digit numbers (`0`, 2 bytes, about 80
// bytes), at sizes just past a power of two, where every table has just doubled. Each such item
// costs its copy, its node or its view, its step, its place in a list and the walk's scratch, and
// each table's geometric growth doubles that. `factor` leaves room above the worst measured.
constexpr std::size_t factor = 128;

// A list of `count` items, `item(i)` each, between `open` and `close`.
template <class Item>
std::string list_of(std::string_view open, std::size_t count, Item item, std::string_view close) {
    std::string text(open);
    for (std::size_t i = 0; i < count; ++i) {
        text += i == 0 ? "" : ",";
        text += item(i);
    }
    text += close;
    return text;
}

// The shapes that hold the most for each byte of their text, each just past a power of two.
void the_smallest_items_stay_within_the_constant() {
    constexpr std::size_t count = 16'385;
    const std::array<std::string, 5> shapes{
        list_of(
            R"({"anyOf": [)", count, [](std::size_t) { return std::string("{}"); }, "]}"),
        list_of(
            R"({"enum": [)", count, [](std::size_t) { return std::string("0"); }, "]}"),
        list_of(
            R"({"anyOf": [)", count, [](std::size_t) { return std::string("true"); }, "]}"),
        list_of(
            R"({"properties": {)", count,
            [](std::size_t i) { return "\"" + std::to_string(i) + "\": {}"; }, "}}"),
        list_of(
            R"({"anyOf": [)", count,
            [](std::size_t i) { return "{\"" + std::to_string(i % 10) + "\": 0}"; }, "]}"),
    };
    for (const std::string& text : shapes) {
        const auto parsed = js::parse(text);
        BOOST_TEST_LE(peak_of(*parsed), factor * text.size());
    }
}

void memory_is_linear_in_the_input() {
    // One long key above many subschemas: each subschema's pointer starts with it.
    const std::string key(10'000, 'k');
    std::string wide = R"({"$defs": {")" + key + R"(": {"anyOf": [true)";
    for (std::size_t i = 1; i < 10'000; ++i) {
        wide += ",true";
    }
    wide += "]}}}";
    const auto parsed_wide = js::parse(wide);
    BOOST_TEST_LE(peak_of(*parsed_wide), factor * wide.size());

    // One long key above many values: each element's view is under it.
    std::string values = R"({"properties": {")" + key + R"(": {"enum": [0)";
    for (std::size_t i = 1; i < 10'000; ++i) {
        values += ",0";
    }
    values += "]}}}";
    const auto parsed_values = js::parse(values);
    BOOST_TEST_LE(peak_of(*parsed_values), factor * values.size());

    // Keys holding `~` and `/`, which a pointer escapes, above an enum beside them.
    std::string escaped = R"({"properties": {)";
    for (std::size_t i = 0; i < 2'000; ++i) {
        escaped += R"("~/)" + std::to_string(i) + R"(": {"type": "null"},)";
    }
    escaped += R"("~last": {"enum": [0)";
    for (std::size_t i = 1; i < 10'000; ++i) {
        escaped += ",0";
    }
    escaped += "]}}}";
    const auto parsed_escaped = js::parse(escaped);
    BOOST_TEST_LE(peak_of(*parsed_escaped), factor * escaped.size());

    // As deep as the default limit, each level under a key of 1,000 bytes: 1 MB of text.
    boost::json::monotonic_resource storage;
    boost::json::value deep = boost::json::object(&storage);
    const std::string level_key(1'000, 'k');
    constexpr std::size_t levels = js::default_max_schema_depth;
    for (std::size_t i = 0; i < levels; ++i) {
        boost::json::object inner(&storage);
        inner[level_key] = std::move(deep);
        boost::json::object outer(&storage);
        outer["properties"] = std::move(inner);
        deep = std::move(outer);
    }
    // each level is the text {"properties":{"<key>": ... }}, and the innermost {}
    const std::size_t deep_size = (levels * (level_key.size() + 20)) + 2;
    BOOST_TEST_LE(peak_of(deep), factor * deep_size);
}

// Checking a document reads each of its parts once: a pointer is spelled only for the fault a
// check returns, so a check that finds none allocates nothing in proportion to the pointers'
// lengths. 2,000 subschemas under a key of 100,000 bytes: a check that spelled each node's
// pointer would hold a string as long as the key.
void checking_spells_no_pointer() {
    const std::string key(100'000, 'k');
    std::string text = R"({"$defs": {")" + key + R"(": {"anyOf": [{})";
    for (std::size_t i = 1; i < 2'000; ++i) {
        text += ",{}";
    }
    text += "]}}}";
    webcpp::test::counting_resource counted;
    const auto d = validator.load(boost::json::parse(text), js::compile_options(),
                                  std::pmr::polymorphic_allocator<std::byte>(&counted));
    if (!BOOST_TEST(d.has_value())) {
        return;
    }
    const std::size_t before = counted.live;
    counted.peak = before;
    BOOST_TEST(d->check().has_value());
    // what a check holds is its scratch, a few kilobytes here, never a pointer's length
    BOOST_TEST_LT(counted.peak - before, key.size() / 10);
    BOOST_TEST_EQ(counted.live, before);
}

}  // namespace

int main() {
    boolean_schemas();
    applicators_keep_source_order();
    dialect_fields_are_explicit();
    sources_are_views_of_the_document();
    anchors_and_unknown_keywords();
    schema_uri_spellings();
    shapes_are_refused_by_place();
    exact_numbers_reach_the_ir();
    every_keyword_of_the_table();
    every_reader_refuses_its_wrong_shapes();
    counts();
    bounds_keep_their_kind();
    anchors();
    ids();
    options();
    text_that_is_not_utf8();
    refusals_come_in_document_order();
    unknown_keywords_hold_data();
    nested_to_the_limit();
    allocations_go_through_the_allocator();
    memory_is_linear_in_the_input();
    checking_spells_no_pointer();
    the_smallest_items_stay_within_the_constant();
    return boost::report_errors();
}
