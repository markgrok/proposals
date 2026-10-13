// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The IR: the kinds' fixed values and the payload each holds, a document built in code and read
// back, values held once and named inside, the check of every invariant it verifies, and copies
// and moves in the allocators they are given.

#include <webcpp/json_schema/detail/json/utf8.hpp>
#include <webcpp/json_schema/ir/document.hpp>

#include "counting_resource.hpp"
#include "recording_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/decimal/decimal128_t.hpp>
#include <boost/decimal/decimal64_t.hpp>
#include <boost/json/monotonic_resource.hpp>
#include <boost/json/parse.hpp>
#include <boost/static_string/static_string.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory_resource>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace js = webcpp::json_schema;
namespace ir = js::ir;
using k = ir::keyword_kind;
using n64 = ir::number<boost::decimal::decimal64_t>;

namespace {

// The pointer `text` spells, added to `d` a step at a token: a token of digits steps into an
// element, any other into a member, `~0` and `~1` read as `~` and `/`.
template <class Document>
ir::pointer_id at(Document& d, std::string_view text) {
    ir::pointer_id pointer = ir::empty_pointer;
    while (!text.empty()) {
        text.remove_prefix(1);
        const std::size_t end = std::min(text.find('/'), text.size());
        const std::string_view token = text.substr(0, end);
        text.remove_prefix(end);
        const bool digits = !token.empty() &&
                            std::ranges::all_of(token, [](char c) { return c >= '0' && c <= '9'; });
        if (digits) {
            pointer =
                d.add_pointer(pointer, static_cast<std::uint32_t>(std::stoul(std::string(token))));
            continue;
        }
        std::string name;
        for (std::size_t i = 0; i < token.size(); ++i) {
            if (token[i] == '~' && i + 1 < token.size()) {
                name.push_back(token[++i] == '0' ? '~' : '/');
            } else {
                name.push_back(token[i]);
            }
        }
        pointer = d.add_pointer(pointer, d.intern(name));
    }
    return pointer;
}

// A pointer of `d`, spelled.
template <class Document>
std::string spelled(const Document& d, ir::pointer_id pointer) {
    std::string out;
    d.write_pointer(pointer, out);
    return out;
}

ir::keyword make(ir::document& d, k kind, std::string_view name, ir::payload data) {
    return {.kind = kind, .name = d.intern(name), .source = ir::no_value, .data = data};
}

// {"type": "object", "properties": {"a": {"minLength": 2}}, "required": ["a"]}, built in code
ir::document example(std::pmr::memory_resource* resource = std::pmr::get_default_resource()) {
    ir::document doc{std::pmr::polymorphic_allocator<std::byte>(resource)};
    const auto root_resource = doc.add_resource({
        .uri = doc.intern("urn:webcpp:root"),
        .source_dialect = js::dialect::draft2020_12,
        .root = ir::node_id{0},
        .parent = ir::resource_id{0},
        .vocabularies = 0x00FF,
        .recursive_anchor = false,
    });
    const ir::node_id root = doc.reserve(root_resource, ir::empty_pointer);
    const ir::node_id a = doc.reserve(root_resource, at(doc, "/properties/a"));
    const std::array entry{ir::named_subschema{.name = doc.intern("a"), .schema = a}};
    const std::array<ir::string_id, 1> required{doc.intern("a")};
    const std::array<ir::keyword, 3> keywords{
        make(doc, k::type, "type",
             ir::types{.set = {.bits = ir::type_set::object}, .integer_by_value = true}),
        make(doc, k::properties, "properties",
             ir::named_subschemas{.entries = doc.add_entries(entry)}),
        make(doc, k::required, "required", ir::names{.strings = doc.add_strings(required)}),
    };
    doc.define(root, ir::node_kind::object, keywords);
    const std::array child{make(doc, k::min_length, "minLength", ir::count{.value = 2})};
    doc.define(a, ir::node_kind::object, child);
    doc.set_root(root);
    return doc;
}

void kinds_are_fixed() {
    BOOST_TEST_EQ(static_cast<int>(k::ref), 1);
    BOOST_TEST_EQ(static_cast<int>(k::all_of), 10);
    BOOST_TEST_EQ(static_cast<int>(k::unevaluated_items), 30);
    BOOST_TEST_EQ(static_cast<int>(k::type), 40);
    BOOST_TEST_EQ(static_cast<int>(k::min_properties), 57);
    BOOST_TEST_EQ(static_cast<int>(k::content_schema), 63);
    BOOST_TEST_EQ(static_cast<int>(k::examples), 76);
    BOOST_TEST_EQ(static_cast<int>(k::external_docs), 83);
    BOOST_TEST_EQ(static_cast<int>(k::user), 92);
    BOOST_TEST_EQ(ir::payload_index(k::properties), 3U);
    BOOST_TEST_EQ(ir::payload_index(k::max_contains), 10U);
    BOOST_TEST_EQ(ir::payload_index(k::user), 16U);
}

void a_document_reads_back_what_was_built() {
    const ir::document doc = example();
    BOOST_TEST(doc.check().has_value());
    BOOST_TEST_EQ(doc.size(), 2U);
    const auto* properties = doc.find(doc.root(), k::properties);
    if (!BOOST_TEST(properties != nullptr)) {
        return;
    }
    const auto entries = doc.entries(std::get<ir::named_subschemas>(properties->data).entries);
    BOOST_TEST_EQ(doc.string(entries[0].name), "a");
    BOOST_TEST_EQ(spelled(doc, doc.at(entries[0].schema).pointer), "/properties/a");
    BOOST_TEST(doc.find(doc.root(), k::pattern) == nullptr);
}

void sources_refer_into_one_copy() {
    webcpp::test::counting_resource counted;
    ir::document doc{std::pmr::polymorphic_allocator<std::byte>(&counted)};
    const auto source = boost::json::parse(R"({"properties": {"a": {"enum": [1, 2, 3]}}})");
    const ir::value_id whole = doc.add_value(source);
    const std::size_t after_copy = counted.peak;
    std::array<ir::value_id, 1000> views{};
    const ir::pointer_id enum_pointer = at(doc, "/properties/a/enum");
    for (auto& v : views) {
        v = doc.add_value_at(whole, enum_pointer);
    }
    const boost::json::array& found = doc.value(views[999]).as_array();
    BOOST_TEST_EQ(found.size(), 3U);
    BOOST_TEST(counted.peak <
               after_copy + (std::size_t{64} * 1024));  // indices only, never another copy
}

void a_deep_value_is_owned_without_recursion() {
    boost::json::monotonic_resource storage;
    boost::json::value deep = boost::json::array(&storage);
    for (int i = 0; i < 100000; ++i) {
        boost::json::array outer(&storage);
        outer.push_back(std::move(deep));
        deep = std::move(outer);
    }
    ir::document doc;
    const ir::value_id v = doc.add_value(deep);
    BOOST_TEST(doc.value(v).is_array());
}  // doc dies here: its values are in a trivially freed arena

void refused(int invariant, const std::function<void(ir::document&)>& broken) {
    ir::document doc = example();
    broken(doc);
    const auto checked = doc.check();
    if (!BOOST_TEST(checked.has_error())) {
        return;
    }
    BOOST_TEST(checked.error().code == js::errc::invalid_ir);
    const std::string expected = "[ir-" + std::to_string(invariant) + "]";
    BOOST_TEST_EQ(std::string(checked.error().what.substr(0, expected.size())), expected);
}

void define_one(ir::document& d, std::string_view pointer, const ir::keyword& keyword) {
    const auto n = d.reserve(ir::resource_id{0}, at(d, pointer));
    const std::array<ir::keyword, 1> keywords{keyword};
    d.define(n, ir::node_kind::object, keywords);
}

void check_refuses_each_broken_invariant() {
    refused(1,
            [](ir::document& d) { (void)d.reserve(ir::resource_id{0}, at(d, "/never/defined")); });
    refused(1, [](ir::document& d) {
        define_one(d, "/x", make(d, k::not_, "not", ir::subschema{.schema = ir::node_id{99}}));
    });
    refused(1, [](ir::document& d) {
        define_one(d, "/y", make(d, k::max_length, "maxLength", ir::text{.value = d.intern("2")}));
    });
    refused(1, [](ir::document& d) {  // defined twice
        const std::array<ir::keyword, 0> none{};
        d.define(d.root(), ir::node_kind::object, none);
    });
    refused(2, [](ir::document& d) {
        define_one(
            d, "/r",
            make(d, k::ref, "$ref", ir::reference{.target = ir::no_node, .uri = d.intern("#/x")}));
    });
    refused(4, [](ir::document& d) {
        const auto n = d.reserve(ir::resource_id{0}, at(d, "/z"));
        const std::array<ir::keyword, 2> twice{
            make(d, k::min_length, "minLength", ir::count{.value = 1}),
            make(d, k::min_length, "minLength", ir::count{.value = 2}),
        };
        d.define(n, ir::node_kind::object, twice);
    });
    refused(6, [](ir::document& d) {
        const auto n = d.reserve(ir::resource_id{0}, at(d, "/w"));
        const std::array<ir::named_subschema, 2> same{
            ir::named_subschema{.name = d.intern("a"), .schema = n},
            ir::named_subschema{.name = d.intern("a"), .schema = n},
        };
        const std::array<ir::keyword, 1> keywords{
            make(d, k::properties, "properties",
                 ir::named_subschemas{.entries = d.add_entries(same)}),
        };
        d.define(n, ir::node_kind::object, keywords);
    });
    refused(7, [](ir::document& d) {
        const auto zero = d.add_number(n64{std::in_place_index<0>, 0});
        define_one(d, "/m", make(d, k::multiple_of, "multipleOf", ir::bound{.value = zero}));
    });
    refused(7, [](ir::document& d) {  // an integer that fits int64_t held as a Number
        const auto two = d.add_number(n64{std::in_place_index<2>, boost::decimal::decimal64_t{2}});
        define_one(d, "/n", make(d, k::maximum, "maximum", ir::bound{.value = two}));
    });
    refused(8, [](ir::document& d) { (void)d.intern("\xC0\x80"); });
    refused(9, [](ir::document& d) {
        (void)d.add_resource({
            .uri = d.intern("urn:webcpp:root"),
            .source_dialect = js::dialect::draft2020_12,
            .root = d.root(),
            .parent = ir::resource_id{0},
            .vocabularies = 0,
            .recursive_anchor = false,
        });
    });
    refused(10, [](ir::document& d) {
        d.add_anchor({
            .resource = ir::resource_id{0},
            .name = d.intern("x"),
            .target = d.root(),
            .dynamic = false,
        });
        d.add_anchor({
            .resource = ir::resource_id{0},
            .name = d.intern("x"),
            .target = d.root(),
            .dynamic = false,
        });
    });
}

void a_copy_with_another_allocator_copies_every_table() {
    const ir::document original = example();
    webcpp::test::counting_resource counted;
    const ir::document copy(original, std::pmr::polymorphic_allocator<std::byte>(&counted));
    BOOST_TEST(counted.allocations > 0);
    BOOST_TEST(copy.check().has_value());
    BOOST_TEST_EQ(copy.size(), original.size());
    BOOST_TEST_EQ(spelled(copy, copy.at(copy.root()).pointer), "");
}

// Spec 5.2's table: the payload type each kind holds.
template <k Kind, class Payload>
constexpr bool holds() {
    constexpr std::size_t index = ir::payload_index(Kind);
    return std::is_same_v<std::variant_alternative_t<index, ir::payload>, Payload>;
}

static_assert(holds<k::ref, ir::reference>() && holds<k::dynamic_ref, ir::dynamic_reference>());
static_assert(holds<k::defs, ir::named_subschemas>() && holds<k::comment, ir::constant>());
static_assert(holds<k::all_of, ir::subschema_list>() && holds<k::any_of, ir::subschema_list>() &&
              holds<k::one_of, ir::subschema_list>());
static_assert(holds<k::not_, ir::subschema>() && holds<k::if_, ir::subschema>() &&
              holds<k::then_, ir::subschema>() && holds<k::else_, ir::subschema>());
static_assert(holds<k::dependent_schemas, ir::named_subschemas>() &&
              holds<k::prefix_items, ir::subschema_list>() && holds<k::items, ir::subschema>() &&
              holds<k::contains, ir::contains_payload>());
static_assert(holds<k::properties, ir::named_subschemas>() &&
              holds<k::pattern_properties, ir::named_subschemas>() &&
              holds<k::additional_properties, ir::subschema>() &&
              holds<k::property_names, ir::subschema>());
static_assert(holds<k::unevaluated_items, ir::subschema>() &&
              holds<k::unevaluated_properties, ir::subschema>());
static_assert(holds<k::type, ir::types>() && holds<k::enum_, ir::constants>() &&
              holds<k::const_, ir::constant>());
static_assert(holds<k::multiple_of, ir::bound>() && holds<k::maximum, ir::bound>() &&
              holds<k::exclusive_maximum, ir::bound>() && holds<k::minimum, ir::bound>() &&
              holds<k::exclusive_minimum, ir::bound>());
static_assert(holds<k::max_length, ir::count>() && holds<k::min_length, ir::count>() &&
              holds<k::max_items, ir::count>() && holds<k::min_items, ir::count>() &&
              holds<k::max_contains, ir::count>() && holds<k::min_contains, ir::count>() &&
              holds<k::max_properties, ir::count>() && holds<k::min_properties, ir::count>());
static_assert(holds<k::pattern, ir::text>() && holds<k::unique_items, ir::constant>() &&
              holds<k::required, ir::names>() && holds<k::dependent_required, ir::dependencies>() &&
              holds<k::format, ir::format_payload>());
static_assert(holds<k::content_encoding, ir::text>() && holds<k::content_media_type, ir::text>() &&
              holds<k::content_schema, ir::subschema>());
static_assert(holds<k::title, ir::constant>() && holds<k::description, ir::constant>() &&
              holds<k::default_, ir::constant>() && holds<k::deprecated, ir::constant>() &&
              holds<k::read_only, ir::constant>() && holds<k::write_only, ir::constant>() &&
              holds<k::examples, ir::constant>());
static_assert(holds<k::discriminator, ir::constant>() && holds<k::example, ir::constant>() &&
              holds<k::xml, ir::constant>() && holds<k::external_docs, ir::constant>());
static_assert(holds<k::unknown, ir::constant>() && holds<k::ignored, ir::constant>() &&
              holds<k::user, ir::user_payload>());
static_assert(std::variant_size_v<ir::payload> == 17);
static_assert(std::is_same_v<std::variant_alternative_t<0, ir::payload>, std::monostate>);
static_assert(ir::type_set::all().bits == 0x7F);

struct wide_traits : js::default_traits {
    using number_type = boost::decimal::decimal128_t;
};

struct recording_traits : js::default_traits {
    using allocator_type = webcpp::test::recording_allocator<std::byte>;
};

static_assert(!std::is_default_constructible_v<ir::basic_document<recording_traits>>);
static_assert(!std::is_copy_constructible_v<ir::document>);
static_assert(std::is_nothrow_move_constructible_v<ir::document>);
static_assert(std::is_nothrow_move_assignable_v<ir::document>);

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

void exactly_the_spec_kinds_hold_a_payload() {
    constexpr std::array<int, 59> kinds{
        1,  2,  3,  4,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 30,
        31, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
        59, 60, 61, 62, 63, 70, 71, 72, 73, 74, 75, 76, 80, 81, 82, 83, 90, 91, 92,
    };
    std::array<bool, 256> listed{};
    for (const int kind : kinds) {
        listed[static_cast<std::size_t>(kind)] = true;
    }
    for (std::size_t v = 0; v < listed.size(); ++v) {
        const std::size_t index = ir::payload_index(static_cast<k>(v));
        BOOST_TEST_EQ(index != 0, listed[v]);
    }
}

void a_type_set_holds_its_bits() {
    const ir::type_set object{.bits = ir::type_set::object};
    BOOST_TEST(object.contains(ir::type_set::object));
    BOOST_TEST(!object.contains(ir::type_set::array));
    BOOST_TEST(!ir::type_set{}.contains(ir::type_set::null));
    for (const auto b : {
             ir::type_set::null,
             ir::type_set::boolean,
             ir::type_set::object,
             ir::type_set::array,
             ir::type_set::number,
             ir::type_set::string,
             ir::type_set::integer,
         }) {
        BOOST_TEST(ir::type_set::all().contains(b));
    }
}

void equal_texts_are_one_string_whose_view_stays() {
    ir::document doc;
    const ir::string_id a = doc.intern("a");
    const std::string_view first = doc.string(a);
    BOOST_TEST(doc.intern("a") == a);
    BOOST_TEST(doc.intern("b") != a);
    BOOST_TEST(doc.intern("") != a);
    BOOST_TEST_EQ(doc.string(doc.intern("")), "");
    for (int i = 0; i < 10000; ++i) {
        (void)doc.intern(std::to_string(i));
    }
    BOOST_TEST(doc.string(a).data() == first.data());
    BOOST_TEST(doc.intern("a") == a);
    BOOST_TEST_EQ(doc.string(doc.intern("9999")), "9999");
}

void a_value_inside_a_value_inside_refers_into_the_owned_one() {
    ir::document doc = example();
    const ir::value_id whole = doc.add_value(boost::json::parse(R"({"a": {"b~/": [1, 2]}})"));
    const ir::value_id a = doc.add_value_at(whole, at(doc, "/a"));
    const ir::value_id two = doc.add_value_at(a, at(doc, "/b~0~1/1"));
    BOOST_TEST_EQ(doc.value(two).as_int64(), 2);
    BOOST_TEST(&doc.value(two) == &doc.value(whole).at_pointer("/a/b~0~1/1"));
    BOOST_TEST(&doc.value(doc.add_value_at(whole, ir::empty_pointer)) == &doc.value(whole));
    BOOST_TEST(doc.check().has_value());
}

void refused(const ir::document& doc, int invariant, std::string_view where = {}) {
    const auto checked = doc.check();
    if (!BOOST_TEST(checked.has_error())) {
        return;
    }
    BOOST_TEST(checked.error().code == js::errc::invalid_ir);
    const std::string expected = "[ir-" + std::to_string(invariant) + "]";
    BOOST_TEST_EQ(std::string(checked.error().what.substr(0, expected.size())), expected);
    if (!where.empty()) {
        const js::diagnostic fault = checked.error();
        BOOST_TEST_EQ(std::string(fault.where.data(), fault.where.size()), std::string(where));
    }
}

void accepted(const std::function<void(ir::document&)>& sound) {
    ir::document doc = example();
    sound(doc);
    const auto checked = doc.check();
    BOOST_TEST(checked.has_value());
    if (checked.has_error()) {
        BOOST_ERROR(checked.error().what.c_str());
    }
}

void check_refuses_values_that_resolve_nowhere() {
    const auto source = boost::json::parse(R"({"enum": [1]})");
    refused(11,
            [&](ir::document& d) { (void)d.add_value_at(d.add_value(source), at(d, "/nope")); });
    refused(11, [&](ir::document& d) {  // an element of an object
        (void)d.add_value_at(d.add_value(source), d.add_pointer(ir::empty_pointer, 0U));
    });
    refused(1, [](ir::document& d) { (void)d.add_value_at(ir::value_id{77}, ir::empty_pointer); });
    refused(1, [](ir::document& d) {
        ir::keyword sourced = make(d, k::min_length, "minLength", ir::count{.value = 1});
        sourced.source = ir::value_id{5};
        define_one(d, "/s", sourced);
    });
    refused(1, [](ir::document& d) {
        define_one(d, "/c", make(d, k::const_, "const", ir::constant{.value = ir::value_id{9}}));
    });
    accepted([&](ir::document& d) {
        const ir::value_id whole = d.add_value(source);
        const std::array first{d.add_value_at(whole, at(d, "/enum/0"))};
        ir::keyword sourced =
            make(d, k::enum_, "enum", ir::constants{.values = d.add_values(first)});
        sourced.source = d.add_value_at(whole, at(d, "/enum"));
        define_one(d, "/e", sourced);
    });
}

// A value named inside the id it is about to receive refers into no value, and neither the check
// nor a copy takes it for one the document owns.
void a_value_inside_itself_is_owned_by_nobody() {
    ir::document doc = example();
    const ir::value_id itself = doc.add_value_at(ir::value_id{0}, ir::empty_pointer);
    BOOST_TEST(itself == ir::value_id{0});
    refused(doc, 1, "values");
    const ir::document copy(doc, std::pmr::polymorphic_allocator<std::byte>());
    refused(copy, 1, "values");
}

void check_refuses_what_names_nothing() {
    const ir::document empty;
    refused(empty, 1, "root");
    ir::document no_resource;
    no_resource.set_root(no_resource.reserve(ir::resource_id{0}, ir::empty_pointer));
    refused(no_resource, 9, "resources");
    refused(1, [](ir::document& d) {
        const std::array<ir::keyword, 0> none{};
        d.define(ir::node_id{50}, ir::node_kind::object, none);
    });
    refused(1, [](ir::document& d) {
        define_one(d, "/u", make(d, static_cast<k>(5), "five", ir::constant{}));
    });
    refused(1, [](ir::document& d) {
        define_one(
            d, "/l",
            make(d, k::all_of, "allOf", ir::subschema_list{.nodes = {.first = 0, .count = 3}}));
    });
    refused(1, [](ir::document& d) {
        const ir::named_subschema entry{.name = d.intern("a"), .schema = ir::node_id{40}};
        const std::array astray{entry};
        define_one(d, "/p",
                   make(d, k::properties, "properties",
                        ir::named_subschemas{.entries = d.add_entries(astray)}));
    });
    refused(1, [](ir::document& d) {
        const std::array<ir::string_id, 1> astray{ir::string_id{9000}};
        define_one(d, "/q",
                   make(d, k::required, "required", ir::names{.strings = d.add_strings(astray)}));
    });
    refused(1, [](ir::document& d) {
        define_one(d, "/b", make(d, k::maximum, "maximum", ir::bound{.value = ir::number_id{3}}));
    });
    refused(1, [](ir::document& d) {
        ir::keyword nameless = make(d, k::min_length, "minLength", ir::count{.value = 1});
        nameless.name = ir::string_id{9000};
        define_one(d, "/n", nameless);
    });
    refused(1, [](ir::document& d) {
        d.add_anchor({
            .resource = ir::resource_id{0},
            .name = d.intern("x"),
            .target = ir::node_id{60},
            .dynamic = false,
        });
    });
}

void check_refuses_each_invariant_by_its_number() {
    refused(2, [](ir::document& d) {
        define_one(d, "/d",
                   make(d, k::dynamic_ref, "$dynamicRef",
                        ir::dynamic_reference{
                            .initial_target = ir::no_node,
                            .uri = d.intern("#meta"),
                            .anchor = d.intern("meta"),
                            .form = ir::dynamic_form::dynamic,
                            .bookended = true,
                        }));
    });
    refused(4, [](ir::document& d) {
        const auto n = d.reserve(ir::resource_id{0}, at(d, "/twice"));
        const auto v = d.add_value(boost::json::value(nullptr));
        const std::array<ir::keyword, 2> twice{
            make(d, k::unknown, "x-a", ir::constant{.value = v}),
            make(d, k::unknown, "x-a", ir::constant{.value = v}),
        };
        d.define(n, ir::node_kind::object, twice);
    });
    accepted([](ir::document& d) {
        const auto n = d.reserve(ir::resource_id{0}, at(d, "/distinct"));
        const auto v = d.add_value(boost::json::value(true));
        const std::array<ir::keyword, 3> distinct{
            make(d, k::unknown, "x-a", ir::constant{.value = v}),
            make(d, k::unknown, "x-b", ir::constant{.value = v}),
            make(d, k::ignored, "x-a", ir::constant{.value = v}),
        };
        d.define(n, ir::node_kind::object, distinct);
    });
    refused(7, [](ir::document& d) {
        const auto negative = d.add_number(n64{std::in_place_index<0>, -3});
        define_one(d, "/m", make(d, k::multiple_of, "multipleOf", ir::bound{.value = negative}));
    });
    refused(7, [](ir::document& d) {
        const auto big =
            d.add_number(n64{std::in_place_index<2>, boost::decimal::decimal64_t{1, 19}});
        define_one(d, "/big", make(d, k::minimum, "minimum", ir::bound{.value = big}));
    });
    accepted([](ir::document& d) {
        const auto half =
            d.add_number(n64{std::in_place_index<2>, boost::decimal::decimal64_t{5, -1}});
        const auto huge =
            d.add_number(n64{std::in_place_index<2>, boost::decimal::decimal64_t{1, 20}});
        const auto top = d.add_number(n64{std::in_place_index<1>, 18446744073709551615U});
        define_one(d, "/h", make(d, k::multiple_of, "multipleOf", ir::bound{.value = half}));
        define_one(d, "/i", make(d, k::maximum, "maximum", ir::bound{.value = huge}));
        define_one(d, "/j",
                   make(d, k::exclusive_minimum, "exclusiveMinimum", ir::bound{.value = top}));
    });
    refused(9, [](ir::document& d) {
        (void)d.add_resource({
            .uri = d.intern("relative/path"),
            .source_dialect = js::dialect::draft7,
            .root = d.reserve(ir::resource_id{1}, ir::empty_pointer),
            .parent = ir::resource_id{0},
            .vocabularies = 0,
            .recursive_anchor = false,
        });
        d.define(ir::node_id{2}, ir::node_kind::always_valid, std::span<const ir::keyword>{});
    });
    refused(9, [](ir::document& d) {
        (void)d.add_resource({
            .uri = d.intern("urn:x#frag"),
            .source_dialect = js::dialect::draft7,
            .root = d.reserve(ir::resource_id{1}, ir::empty_pointer),
            .parent = ir::resource_id{0},
            .vocabularies = 0,
            .recursive_anchor = false,
        });
        d.define(ir::node_id{2}, ir::node_kind::always_valid, std::span<const ir::keyword>{});
    });
    refused(9, [](ir::document& d) {
        (void)d.add_resource({
            .uri = d.intern("urn:x"),
            .source_dialect = js::dialect::draft7,
            .root = d.reserve(ir::resource_id{1}, at(d, "/not-the-root")),
            .parent = ir::resource_id{0},
            .vocabularies = 0,
            .recursive_anchor = false,
        });
        d.define(ir::node_id{2}, ir::node_kind::always_valid, std::span<const ir::keyword>{});
    });
    accepted([](ir::document& d) {
        (void)d.add_resource({
            .uri = d.intern("https://example.com/b.json"),
            .source_dialect = js::dialect::draft7,
            .root = d.reserve(ir::resource_id{1}, ir::empty_pointer),
            .parent = ir::resource_id{0},
            .vocabularies = 0,
            .recursive_anchor = false,
        });
        d.define(ir::node_id{2}, ir::node_kind::always_valid, std::span<const ir::keyword>{});
        d.add_anchor({
            .resource = ir::resource_id{1},
            .name = d.intern("x"),
            .target = ir::node_id{2},
            .dynamic = false,
        });
        d.add_anchor({
            .resource = ir::resource_id{0},
            .name = d.intern("x"),
            .target = d.root(),
            .dynamic = false,
        });
        d.add_anchor({
            .resource = ir::resource_id{0},
            .name = d.intern("x"),
            .target = d.root(),
            .dynamic = true,
        });
    });
    refused(10, [](ir::document& d) {
        (void)d.add_resource({
            .uri = d.intern("urn:other"),
            .source_dialect = js::dialect::draft7,
            .root = d.reserve(ir::resource_id{1}, ir::empty_pointer),
            .parent = ir::resource_id{0},
            .vocabularies = 0,
            .recursive_anchor = false,
        });
        d.define(ir::node_id{2}, ir::node_kind::always_valid, std::span<const ir::keyword>{});
        d.add_anchor({
            .resource = ir::resource_id{0},
            .name = d.intern("y"),
            .target = ir::node_id{2},
            .dynamic = false,
        });
    });
}

void a_fault_names_its_place_and_stays_utf8() {
    ir::document doc = example();
    std::string name;
    for (int i = 0; i < 400; ++i) {
        name += "\xC3\xA9";
    }
    define_one(doc, "/long", make(doc, k::max_length, name, ir::text{}));
    const auto checked = doc.check();
    if (!BOOST_TEST(checked.has_error())) {
        return;
    }
    // error() returns the diagnostic by value: the views below need it held.
    const js::diagnostic fault = checked.error();
    const std::string_view what(fault.what.data(), fault.what.size());
    BOOST_TEST(what.starts_with("[ir-1] a payload that is not its kind's: \xC3\xA9"));
    BOOST_TEST(what.ends_with("..."));
    BOOST_TEST(js::detail::is_valid_utf8(what));
    BOOST_TEST_EQ(std::string(fault.where.data(), fault.where.size()), "/long");
}

void many_entries_are_checked_for_one_name_twice() {
    ir::document doc = example();
    const ir::node_id n = doc.reserve(ir::resource_id{0}, at(doc, "/wide"));
    std::vector<ir::named_subschema> entries;
    entries.reserve(20001);
    for (int i = 0; i < 20000; ++i) {
        entries.push_back({.name = doc.intern("p" + std::to_string(i)), .schema = n});
    }
    std::array<ir::keyword, 1> keywords{
        make(doc, k::properties, "properties",
             ir::named_subschemas{.entries = doc.add_entries(entries)}),
    };
    doc.define(n, ir::node_kind::object, keywords);
    BOOST_TEST(doc.check().has_value());

    ir::document twice = example();
    const ir::node_id m = twice.reserve(ir::resource_id{0}, at(twice, "/wide"));
    entries.clear();
    for (int i = 0; i < 20000; ++i) {
        entries.push_back({.name = twice.intern("p" + std::to_string(i)), .schema = m});
    }
    entries.push_back({.name = twice.intern("p17"), .schema = m});
    keywords[0] = make(twice, k::properties, "properties",
                       ir::named_subschemas{.entries = twice.add_entries(entries)});
    twice.define(m, ir::node_kind::object, keywords);
    refused(twice, 6, "/wide");
}

void a_copy_owns_its_values_in_its_allocator() {
    webcpp::test::counting_resource first;
    webcpp::test::counting_resource second;
    {
        auto original = std::make_unique<ir::document>(example(&first));
        const ir::value_id whole = original->add_value(boost::json::parse(R"({"a": [1, "two"]})"));
        const ir::value_id two = original->add_value_at(whole, at(*original, "/a/1"));
        (void)original->add_value_at(whole, at(*original, "/b"));
        const ir::document copy(*original, std::pmr::polymorphic_allocator<std::byte>(&second));
        BOOST_TEST(copy.get_allocator().resource() == &second);
        BOOST_TEST(&copy.value(two) != &original->value(two));
        original.reset();
        BOOST_TEST_EQ(first.live, 0U);
        BOOST_TEST_EQ(copy.value(two).as_string(), "two");
        BOOST_TEST(&copy.value(two) == &copy.value(whole).at_pointer("/a/1"));
        refused(copy, 11, "/b");
    }
    BOOST_TEST_EQ(second.live, 0U);
}

void a_copy_interns_as_its_original() {
    ir::document original = example();
    ir::document copy(original, std::pmr::polymorphic_allocator<std::byte>());
    const ir::keyword* required = original.find(original.root(), k::required);
    if (!BOOST_TEST(required != nullptr)) {
        return;
    }
    BOOST_TEST(copy.intern("a") ==
               original.strings(std::get<ir::names>(required->data).strings)[0]);
    BOOST_TEST(copy.intern("required") == required->name);
    const ir::string_id fresh = copy.intern("fresh");
    BOOST_TEST(fresh == original.intern("fresh"));
    BOOST_TEST_EQ(copy.string(fresh), "fresh");
}

void moves_keep_every_view_and_free_everything() {
    webcpp::test::counting_resource first;
    webcpp::test::counting_resource second;
    {
        ir::document source = example(&first);
        const ir::value_id whole = source.add_value(boost::json::parse(R"({"a": [1, 2]})"));
        const ir::value_id one = source.add_value_at(whole, at(source, "/a/0"));
        const boost::json::value* address = &source.value(one);
        const char* text = source.string(source.intern("a")).data();
        ir::document moved(std::move(source));
        BOOST_TEST(&moved.value(one) == address);
        BOOST_TEST(moved.string(moved.intern("a")).data() == text);
        ir::document target = example(&second);
        (void)target.add_value(boost::json::parse("[[[]]]"));
        target = std::move(moved);
        BOOST_TEST_EQ(second.live, 0U);
        BOOST_TEST(&target.value(one) == address);
        BOOST_TEST(target.get_allocator().resource() == &first);
        BOOST_TEST(target.check().has_value());
    }
    BOOST_TEST_EQ(first.live, 0U);
    BOOST_TEST_EQ(second.live, 0U);
}

void nothing_reaches_the_default_resource() {
    const auto source = boost::json::parse(R"({"enum": [1, {"b": null}]})");
    webcpp::test::counting_resource elsewhere;
    webcpp::test::counting_resource used;
    webcpp::test::counting_resource copied;
    {
        const default_resource_guard guard(&elsewhere);
        ir::document doc = example(&used);
        (void)doc.add_value_at(doc.add_value(source), at(doc, "/enum/1"));
        const ir::document copy(doc, std::pmr::polymorphic_allocator<std::byte>(&copied));
        BOOST_TEST(doc.check().has_value());
        BOOST_TEST(copy.check().has_value());
    }
    BOOST_TEST_EQ(elsewhere.allocations, 0U);
    BOOST_TEST(used.allocations > 0);
    BOOST_TEST(copied.allocations > 0);
}

void a_non_pmr_allocator_gets_every_block_back() {
    webcpp::test::recording_resource first;
    webcpp::test::recording_resource second;
    {
        using doc_type = ir::basic_document<recording_traits>;
        doc_type doc{recording_traits::allocator_type(&first)};
        const auto root_resource = doc.add_resource({
            .uri = doc.intern("urn:webcpp:root"),
            .source_dialect = js::dialect::draft4,
            .root = ir::node_id{0},
            .parent = ir::resource_id{0},
            .vocabularies = 0,
            .recursive_anchor = false,
        });
        const ir::node_id root = doc.reserve(root_resource, ir::empty_pointer);
        const ir::value_id v = doc.add_value(
            boost::json::parse(R"({"const": "a long string, past any small buffer"})"));
        const std::array<ir::keyword, 1> keywords{
            ir::keyword{
                .kind = k::const_,
                .name = doc.intern("const"),
                .source = v,
                .data = ir::constant{.value = doc.add_value_at(v, at(doc, "/const"))},
            },
        };
        doc.define(root, ir::node_kind::object, keywords);
        doc.set_root(root);
        BOOST_TEST(doc.check().has_value());
        const doc_type copy(doc, recording_traits::allocator_type(&second));
        BOOST_TEST(copy.check().has_value());
        BOOST_TEST_EQ(copy.value(ir::value_id{1}).as_string(),
                      "a long string, past any small buffer");
        BOOST_TEST(second.allocations > 0);
    }
    for (const auto* r : {&first, &second}) {
        BOOST_TEST_EQ(r->live_blocks(), 0U);
        BOOST_TEST_EQ(r->mismatched, 0U);
        BOOST_TEST_EQ(r->misaligned, 0U);
    }
}

// A document of `Traits` whose root holds one keyword of `kind` on the number `value`.
template <class Traits>
js::result<void> check_bound(k kind, typename Traits::number_type value) {
    ir::basic_document<Traits> doc;
    const auto r = doc.add_resource({
        .uri = doc.intern("urn:webcpp:root"),
        .source_dialect = js::dialect::draft2020_12,
        .root = ir::node_id{0},
        .parent = ir::resource_id{0},
        .vocabularies = 0,
        .recursive_anchor = false,
    });
    const ir::node_id root = doc.reserve(r, ir::empty_pointer);
    const ir::number_id n =
        doc.add_number(ir::number<typename Traits::number_type>{std::in_place_index<2>, value});
    const ir::keyword keyword{
        .kind = kind,
        .name = doc.intern("bound"),
        .source = ir::no_value,
        .data = ir::bound{.value = n},
    };
    const std::array keywords{keyword};
    doc.define(root, ir::node_kind::object, keywords);
    doc.set_root(root);
    return doc.check();
}

// Invariant 7: every bound and divisor is finite, for each number type the traits allow.
template <class Traits>
void a_bound_that_is_not_finite_is_refused() {
    using number = Traits::number_type;
    using limits = std::numeric_limits<number>;
    for (const k kind :
         {k::multiple_of, k::maximum, k::exclusive_maximum, k::minimum, k::exclusive_minimum}) {
        for (const number value : {limits::quiet_NaN(), limits::infinity(), -limits::infinity()}) {
            const auto checked = check_bound<Traits>(kind, value);
            if (!BOOST_TEST(checked.has_error())) {
                continue;
            }
            const js::diagnostic fault = checked.error();
            BOOST_TEST_EQ(std::string(fault.what.data(), fault.what.size()),
                          "[ir-7] a number that is not finite: bound");
        }
        BOOST_TEST(check_bound<Traits>(kind, number{25, -1}).has_value());
    }
}

// A value named one step inside a view resolves from that view; a step of the wrong kind, a
// member of an array or an element of an object, names nothing, here and in a copy.
void a_step_of_the_wrong_kind_names_nothing() {
    ir::document doc = example();
    const ir::value_id whole =
        doc.add_value(boost::json::parse(R"({"a": {"b": [1, 2]}, "ab": "sibling"})"));
    const ir::value_id a = doc.add_value_at(whole, at(doc, "/a"));
    const ir::value_id b = doc.add_value_at(a, at(doc, "/b"));
    BOOST_TEST_EQ(doc.value(doc.add_value_at(b, at(doc, "/1"))).as_int64(), 2);
    BOOST_TEST(doc.check().has_value());
    (void)doc.add_value_at(a, doc.add_pointer(ir::empty_pointer, 0U));
    refused(doc, 11, "/0");
    const ir::document copy(doc, std::pmr::polymorphic_allocator<std::byte>());
    refused(copy, 11, "/0");
    ir::document member_of_array = example();
    const ir::value_id list = member_of_array.add_value(boost::json::parse("[1]"));
    (void)member_of_array.add_value_at(list, at(member_of_array, "/x"));
    refused(member_of_array, 11, "/x");
}

// A pointer is spelled from its steps where it is reported, escaped as RFC 6901 writes it, and
// its steps hold each token once however many pointers extend them.
void pointers_are_steps_spelled_on_demand() {
    webcpp::test::counting_resource counted;
    ir::document doc{std::pmr::polymorphic_allocator<std::byte>(&counted)};
    const ir::pointer_id p = at(doc, "/a~1b/0/~0/");
    BOOST_TEST_EQ(spelled(doc, p), "/a~1b/0/~0/");
    BOOST_TEST_EQ(spelled(doc, ir::empty_pointer), "");
    BOOST_TEST(doc.step(p).element == false);
    BOOST_TEST_EQ(doc.string(doc.step(p).name), "");
    BOOST_TEST(doc.step(doc.step(p).parent).name == doc.intern("~"));
    boost::static_strings::static_string<256> fixed;
    doc.write_pointer(at(doc, "/x/12"), fixed);
    BOOST_TEST_EQ(std::string(fixed.data(), fixed.size()), "/x/12");

    // 10,000 pointers under one key of 10,000 bytes: the key once, a step each.
    const ir::pointer_id parent =
        doc.add_pointer(ir::empty_pointer, doc.intern(std::string(10'000, 'k')));
    const std::size_t before = counted.live;
    for (std::uint32_t i = 0; i < 10'000; ++i) {
        (void)doc.add_pointer(parent, i);
    }
    BOOST_TEST_LE(counted.live - before, std::size_t{10'000} * 64);
    BOOST_TEST_EQ(spelled(doc, doc.add_pointer(parent, 7U)).size(), 10'003U);
}

// A step whose name the document does not hold ends a pointer where it is spelled, as a step whose
// parent comes after it does: nothing is read or written past the room the spelling sized.
void a_pointer_ends_at_a_step_it_cannot_spell() {
    ir::document doc;
    const ir::pointer_id unheld = doc.add_pointer(ir::empty_pointer, ir::string_id{999});
    const ir::pointer_id below = doc.add_pointer(unheld, doc.intern("a"));
    BOOST_TEST_EQ(spelled(doc, unheld), "");
    BOOST_TEST_EQ(spelled(doc, below), "/a");
    boost::static_strings::static_string<4> tight;
    doc.write_pointer(below, tight);
    BOOST_TEST_EQ(std::string(tight.data(), tight.size()), "/a");
}

// A step that extends a pointer after it is no pointer (invariant 15).
void a_step_extends_a_pointer_before_it() {
    refused(15, [](ir::document& d) {
        const ir::pointer_id first = d.add_pointer(ir::empty_pointer, d.intern("a"));
        (void)d.add_pointer(ir::pointer_id{static_cast<std::uint32_t>(first) + 1}, 0U);
    });
    refused(1, [](ir::document& d) { (void)d.add_pointer(ir::empty_pointer, ir::string_id{999}); });
    refused(1, [](ir::document& d) {
        const auto n = d.reserve(ir::resource_id{0}, ir::pointer_id{500});
        d.define(n, ir::node_kind::always_valid, {});
    });
}

}  // namespace

int main() {
    kinds_are_fixed();
    a_document_reads_back_what_was_built();
    sources_refer_into_one_copy();
    a_deep_value_is_owned_without_recursion();
    check_refuses_each_broken_invariant();
    a_copy_with_another_allocator_copies_every_table();
    exactly_the_spec_kinds_hold_a_payload();
    a_type_set_holds_its_bits();
    equal_texts_are_one_string_whose_view_stays();
    a_value_inside_a_value_inside_refers_into_the_owned_one();
    check_refuses_values_that_resolve_nowhere();
    a_value_inside_itself_is_owned_by_nobody();
    check_refuses_what_names_nothing();
    check_refuses_each_invariant_by_its_number();
    a_fault_names_its_place_and_stays_utf8();
    many_entries_are_checked_for_one_name_twice();
    a_copy_owns_its_values_in_its_allocator();
    a_copy_interns_as_its_original();
    moves_keep_every_view_and_free_everything();
    nothing_reaches_the_default_resource();
    a_non_pmr_allocator_gets_every_block_back();
    a_bound_that_is_not_finite_is_refused<js::default_traits>();
    a_bound_that_is_not_finite_is_refused<wide_traits>();
    a_step_of_the_wrong_kind_names_nothing();
    pointers_are_steps_spelled_on_demand();
    a_pointer_ends_at_a_step_it_cannot_spell();
    a_step_extends_a_pointer_before_it();
    return boost::report_errors();
}
