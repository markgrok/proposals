// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/detail/output/record.hpp>
#include <webcpp/json_schema/detail/vm/collecting_sink.hpp>
#include <webcpp/json_schema/detail/vm/flag_sink.hpp>

#include "counting_resource.hpp"

#include <boost/core/lightweight_test.hpp>
#include <boost/json/monotonic_resource.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace js = webcpp::json_schema;
namespace detail = js::detail;

static_assert(detail::evaluation_sink<detail::flag_sink>);
static_assert(detail::evaluation_sink<detail::collecting_sink>);
static_assert(!detail::flag_sink::records);
static_assert(detail::collecting_sink::records);

namespace {

const boost::json::value schema_value = 3;

// The instance's value, built at its first use: its string may allocate.
const boost::json::value& instance_value() {
    static const boost::json::value value = boost::json::string_view("abcd");
    return value;
}

js::error make_error(std::string_view keyword_location) {
    return detail::error_access::make_error({
        .kind = js::error_kind::max_length,
        .keyword = "maxLength",
        .instance_location = "/name",
        .keyword_location = keyword_location,
        .absolute_keyword_location = "urn:webcpp:root#/maxLength",
        .expected = &schema_value,
        .received = &instance_value(),
    });
}

js::annotation make_annotation(std::string_view keyword_location = "/title",
                               const boost::json::value* value = &schema_value) {
    return detail::error_access::make_annotation({
        .keyword = "title",
        .instance_location = "",
        .keyword_location = keyword_location,
        .absolute_keyword_location = "urn:webcpp:root#/title",
        .value = value,
    });
}

detail::event_context at(std::span<const detail::path_token> evaluation) {
    return {
        .evaluation_path = evaluation,
        .instance_path = {},
        .resource_uri = "urn:webcpp:root",
        .schema_pointer = "",
    };
}

void error_kinds_are_fixed() {
    BOOST_TEST_EQ(static_cast<int>(js::error_kind::false_schema), 1);
    BOOST_TEST_EQ(static_cast<int>(js::error_kind::type), 2);
    BOOST_TEST_EQ(static_cast<int>(js::error_kind::pattern_limit), 13);
    BOOST_TEST_EQ(static_cast<int>(js::error_kind::unevaluated_properties), 33);
    BOOST_TEST_EQ(static_cast<int>(js::error_kind::reference), 39);
    BOOST_TEST_EQ(static_cast<int>(js::error_kind::depth_limit), 43);
}

// Every kind, in the order of its value, so that no two kinds can swap unseen.
void every_error_kind_has_its_value() {
    using k = js::error_kind;
    const std::array kinds{
        k::false_schema,
        k::type,
        k::enum_value,
        k::const_value,
        k::multiple_of,
        k::maximum,
        k::exclusive_maximum,
        k::minimum,
        k::exclusive_minimum,
        k::max_length,
        k::min_length,
        k::pattern,
        k::pattern_limit,
        k::format,
        k::max_items,
        k::min_items,
        k::unique_items,
        k::contains,
        k::min_contains,
        k::max_contains,
        k::max_properties,
        k::min_properties,
        k::required,
        k::dependent_required,
        k::properties,
        k::pattern_properties,
        k::additional_properties,
        k::property_names,
        k::dependent_schemas,
        k::prefix_items,
        k::items,
        k::unevaluated_items,
        k::unevaluated_properties,
        k::all_of,
        k::any_of,
        k::one_of,
        k::not_schema,
        k::if_then_else,
        k::reference,
        k::content,
        k::schema,
        k::user,
        k::depth_limit,
    };
    BOOST_TEST_EQ(kinds.size(), 43U);
    for (std::size_t i = 0; i < kinds.size(); ++i) {
        BOOST_TEST_EQ(static_cast<std::size_t>(kinds[i]), i + 1);
    }
    BOOST_TEST_EQ(static_cast<int>(js::collect::errors), 0);
    BOOST_TEST_EQ(static_cast<int>(js::collect::errors_and_annotations), 1);
    BOOST_TEST_EQ(static_cast<int>(js::collect::everything), 2);
}

void errors_are_copied_and_located() {
    std::pmr::monotonic_buffer_resource resource;
    detail::record record(resource, js::collect::errors);
    detail::collecting_sink sink(record);
    const std::array<detail::path_token, 2> path{
        detail::path_token{.name = "properties"},
        detail::path_token{.name = "a/b"},
    };
    sink.enter(at(path));
    {
        std::string scratch = "/properties/name/maxLength";  // freed before the record is read
        sink.fail(make_error(scratch));
        scratch.assign(scratch.size(), 'x');
    }
    sink.leave(at(path), false);
    record.finish();
    if (!BOOST_TEST_EQ(record.errors().size(), 1U)) {
        return;
    }
    BOOST_TEST_EQ(record.errors()[0].keyword_location(), "/properties/name/maxLength");
    BOOST_TEST(&record.errors()[0].received() == &instance_value());
    BOOST_TEST_EQ(record.units()[0].keyword_location, "/properties/a~1b");
    BOOST_TEST(!record.units()[0].valid);
}

void a_failed_node_drops_its_annotations_and_keeps_its_errors() {
    std::pmr::monotonic_buffer_resource resource;
    detail::record record(resource, js::collect::errors_and_annotations);
    detail::collecting_sink sink(record);
    sink.enter(at({}));
    sink.annotate(make_annotation());
    sink.fail(make_error("/maxLength"));
    sink.leave(at({}), false);
    record.finish();
    BOOST_TEST_EQ(record.annotations().size(), 0U);
    BOOST_TEST_EQ(record.errors().size(), 1U);
}

void discard_truncates_unless_everything_is_collected() {
    for (const js::collect what : {js::collect::errors_and_annotations, js::collect::everything}) {
        std::pmr::monotonic_buffer_resource resource;
        detail::record record(resource, what);
        detail::collecting_sink sink(record);
        sink.enter(at({}));
        sink.annotate(make_annotation());
        const auto mark = sink.checkpoint();
        sink.enter(at({}));
        sink.fail(make_error("/anyOf/0/maxLength"));
        sink.leave(at({}), false);
        sink.discard(mark);
        sink.leave(at({}), true);
        record.finish();
        BOOST_TEST_EQ(record.errors().size(), 0U);
        BOOST_TEST_EQ(record.annotations().size(), 1U);
        if (what == js::collect::everything) {
            BOOST_TEST_EQ(record.units().size(), 2U);  // the failed branch stays, marked
            BOOST_TEST(!record.units()[1].counts && !record.units()[1].valid);
            BOOST_TEST_EQ(record.recorded_errors().size(), 1U);
            BOOST_TEST_EQ(record.recorded_error_counts()[0], 0U);
        } else {
            BOOST_TEST_EQ(record.units().size(), 1U);
        }
    }
}

void collect_errors_records_no_annotation() {
    std::pmr::monotonic_buffer_resource resource;
    detail::record record(resource, js::collect::errors);
    detail::collecting_sink sink(record);
    sink.enter(at({}));
    sink.annotate(make_annotation());
    sink.leave(at({}), true);
    record.finish();
    BOOST_TEST_EQ(record.annotations().size(), 0U);
    BOOST_TEST_EQ(record.recorded_annotations().size(), 0U);
    BOOST_TEST(record.collected() == js::collect::errors);
}

// Property names that are not plain identifiers, and indices up to the largest, give
// well-formed JSON Pointers; the absolute location is the resource, "#" and the node's pointer.
// Its units are valid and record nothing, which only collect::everything keeps.
void locations_are_json_pointers() {
    std::pmr::monotonic_buffer_resource resource;
    detail::record record(resource, js::collect::everything);
    detail::collecting_sink sink(record);
    const std::array<detail::path_token, 5> evaluation{
        detail::path_token{.name = "properties"},
        detail::path_token{.name = "~1"},
        detail::path_token{.name = ""},
        detail::path_token{.name = "", .index = 7, .is_index = true},
        detail::path_token{.name = "~/"},
    };
    const std::array<detail::path_token, 4> instance{
        detail::path_token{.name = std::string_view("a\0b", 3)},
        detail::path_token{.name = "", .index = 18446744073709551615ULL, .is_index = true},
        detail::path_token{.name = "\xF0\x9F\x90\xB2"},
        detail::path_token{.name = "", .index = 0, .is_index = true},
    };
    const detail::event_context where{
        .evaluation_path = evaluation,
        .instance_path = instance,
        .resource_uri = "https://example.com/s.json",
        .schema_pointer = "/$defs/a~1b",
    };
    sink.enter(where);
    sink.leave(where, true);
    sink.enter(at({}));
    sink.leave(at({}), true);
    record.finish();
    if (!BOOST_TEST_EQ(record.units().size(), 2U)) {
        return;
    }
    const detail::unit& u = record.units()[0];
    BOOST_TEST_EQ(u.keyword_location, "/properties/~01//7/~0~1");
    BOOST_TEST_EQ(u.instance_location,
                  std::string_view("/a\0b/18446744073709551615/\xF0\x9F\x90\xB2/0", 32));
    BOOST_TEST_EQ(u.absolute_keyword_location, "https://example.com/s.json#/$defs/a~1b");
    BOOST_TEST(u.valid && u.counts);
    BOOST_TEST_EQ(record.units()[1].keyword_location, "");
    BOOST_TEST_EQ(record.units()[1].instance_location, "");
    BOOST_TEST_EQ(record.units()[1].absolute_keyword_location, "urn:webcpp:root#");
}

// Unit 1 is valid and records nothing, which only collect::everything keeps.
void units_have_parents_and_results() {
    std::pmr::monotonic_buffer_resource resource;
    detail::record record(resource, js::collect::everything);
    detail::collecting_sink sink(record);
    BOOST_TEST_EQ(record.current_unit(), detail::no_unit);
    sink.enter(at({}));
    BOOST_TEST_EQ(record.current_unit(), 0U);
    sink.enter(at({}));
    sink.leave(at({}), true);
    sink.enter(at({}));
    BOOST_TEST_EQ(record.current_unit(), 2U);
    sink.enter(at({}));
    sink.fail(make_error("/a/b/c"));
    sink.leave(at({}), false);
    BOOST_TEST_EQ(record.current_unit(), 2U);
    sink.fail(make_error("/a/b"));
    sink.leave(at({}), false);
    BOOST_TEST_EQ(record.current_unit(), 0U);
    sink.leave(at({}), false);
    BOOST_TEST_EQ(record.current_unit(), detail::no_unit);
    record.finish();
    if (!BOOST_TEST_EQ(record.units().size(), 4U) || !BOOST_TEST_EQ(record.errors().size(), 2U)) {
        return;
    }
    const std::array<std::uint32_t, 4> parents{detail::no_unit, 0, 0, 2};
    const std::array<bool, 4> valid{false, true, false, false};
    for (std::size_t i = 0; i < parents.size(); ++i) {
        BOOST_TEST_EQ(record.units()[i].parent, parents[i]);
        BOOST_TEST_EQ(record.units()[i].valid, valid[i]);
        BOOST_TEST(record.units()[i].counts);
    }
    BOOST_TEST_EQ(record.errors()[0].keyword_location(), "/a/b/c");
    BOOST_TEST_EQ(record.error_units()[0], 3U);
    BOOST_TEST_EQ(record.errors()[1].keyword_location(), "/a/b");
    BOOST_TEST_EQ(record.error_units()[1], 2U);
}

// P2 drops what the failed node and its descendants annotated, and nothing before it.
void a_failed_node_drops_its_descendants_annotations() {
    for (const js::collect what : {js::collect::errors_and_annotations, js::collect::everything}) {
        std::pmr::monotonic_buffer_resource resource;
        detail::record record(resource, what);
        detail::collecting_sink sink(record);
        sink.enter(at({}));
        sink.annotate(make_annotation("/root"));
        sink.enter(at({}));
        sink.annotate(make_annotation("/first"));
        sink.leave(at({}), true);
        sink.enter(at({}));
        sink.annotate(make_annotation("/second"));
        sink.enter(at({}));
        sink.annotate(make_annotation("/second/inner"));
        sink.leave(at({}), true);
        sink.leave(at({}), false);
        sink.annotate(make_annotation("/root/after"));
        sink.leave(at({}), true);
        record.finish();
        if (!BOOST_TEST_EQ(record.annotations().size(), 3U)) {
            continue;
        }
        BOOST_TEST_EQ(record.annotations()[0].keyword_location(), "/root");
        BOOST_TEST_EQ(record.annotations()[1].keyword_location(), "/first");
        BOOST_TEST_EQ(record.annotations()[2].keyword_location(), "/root/after");
        BOOST_TEST_EQ(record.annotation_units()[0], 0U);
        BOOST_TEST_EQ(record.annotation_units()[1], 1U);
        BOOST_TEST_EQ(record.annotation_units()[2], 0U);
        if (what == js::collect::everything) {
            const std::array<std::uint8_t, 5> counts{1, 1, 0, 0, 1};
            BOOST_TEST_ALL_EQ(record.recorded_annotation_counts().begin(),
                              record.recorded_annotation_counts().end(), counts.begin(),
                              counts.end());
            const std::array<std::uint32_t, 5> units{0, 1, 2, 3, 0};
            BOOST_TEST_ALL_EQ(record.recorded_annotation_units().begin(),
                              record.recorded_annotation_units().end(), units.begin(), units.end());
            BOOST_TEST(record.units()[2].counts);  // P2 drops annotations, never a unit
        } else {
            BOOST_TEST_EQ(record.recorded_annotations().size(), 3U);
        }
    }
}

// Nested discards come in LIFO order; every collect gives the same verdicts, and only
// collect::everything keeps the discarded records, flagged.
void discards_nest_under_every_collect() {
    for (const js::collect what :
         {js::collect::errors, js::collect::errors_and_annotations, js::collect::everything}) {
        std::pmr::monotonic_buffer_resource resource;
        detail::record record(resource, what);
        detail::collecting_sink sink(record);
        sink.enter(at({}));  // unit 0
        const auto outer = sink.checkpoint();
        sink.enter(at({}));  // unit 1, discarded
        sink.fail(make_error("/anyOf/0/minimum"));
        const auto inner = sink.checkpoint();
        sink.enter(at({}));  // unit 2, discarded
        sink.fail(make_error("/anyOf/0/anyOf/0/type"));
        sink.leave(at({}), false);
        sink.discard(inner);
        sink.leave(at({}), false);
        sink.discard(outer);
        sink.enter(at({}));  // unit 3 in everything, else 1
        sink.annotate(make_annotation("/anyOf/1/title"));
        sink.fail(make_error("/anyOf/1/type"));
        sink.leave(at({}), false);
        sink.leave(at({}), false);
        record.finish();
        const bool everything = what == js::collect::everything;
        const std::uint32_t last = everything ? 3 : 1;
        if (!BOOST_TEST_EQ(record.errors().size(), 1U)) {
            continue;
        }
        BOOST_TEST_EQ(record.errors()[0].keyword_location(), "/anyOf/1/type");
        BOOST_TEST_EQ(record.error_units()[0], last);
        BOOST_TEST_EQ(record.annotations().size(), 0U);  // its node failed
        BOOST_TEST_EQ(record.units().size(), everything ? 4U : 2U);
        BOOST_TEST_EQ(record.units()[last].parent, 0U);
        if (everything) {
            const std::array<bool, 4> counts{true, false, false, true};
            for (std::size_t i = 0; i < counts.size(); ++i) {
                BOOST_TEST_EQ(record.units()[i].counts, counts[i]);
            }
            BOOST_TEST_EQ(record.units()[2].parent, 1U);
            const std::array<std::uint8_t, 3> error_counts{0, 0, 1};
            BOOST_TEST_ALL_EQ(record.recorded_error_counts().begin(),
                              record.recorded_error_counts().end(), error_counts.begin(),
                              error_counts.end());
            const std::array<std::uint32_t, 3> error_units{1, 2, 3};
            BOOST_TEST_ALL_EQ(record.recorded_error_units().begin(),
                              record.recorded_error_units().end(), error_units.begin(),
                              error_units.end());
            BOOST_TEST_EQ(record.recorded_annotations().size(), 1U);
            BOOST_TEST_EQ(record.recorded_annotation_counts()[0], 0U);
        } else {
            BOOST_TEST_EQ(record.recorded_errors().size(), 1U);
            BOOST_TEST_EQ(record.recorded_annotations().size(), 0U);
        }
    }
}

// A node nested past the sink's inline stack keeps P2 right: the annotations made inside the
// one failed node, at depth 50, and below it are dropped; the 50 above it stay.
void deep_nesting_keeps_p2() {
    std::pmr::monotonic_buffer_resource resource;
    detail::record record(resource, js::collect::errors_and_annotations);
    detail::collecting_sink sink(record);
    constexpr int depth = 100;
    for (int i = 0; i < depth; ++i) {
        sink.enter(at({}));
        sink.annotate(make_annotation());
    }
    for (int i = depth - 1; i >= 0; --i) {
        sink.leave(at({}), i != 50);
    }
    record.finish();
    BOOST_TEST_EQ(record.annotations().size(), 50U);
    BOOST_TEST_EQ(record.units().size(), 100U);
    BOOST_TEST_EQ(record.current_unit(), detail::no_unit);
}

void details_are_copied() {
    std::pmr::monotonic_buffer_resource resource;
    detail::record record(resource, js::collect::errors);
    detail::collecting_sink sink(record);
    sink.enter(at({}));
    {
        std::vector<std::string> names{"first", "a name long enough to leave the small buffer", ""};
        const std::array<std::string_view, 3> views{names[0], names[1], names[2]};
        std::array<std::size_t, 2> indices{3, 5};
        const auto e = make_error("/required");
        auto fields = detail::error_fields{
            .kind = js::error_kind::required,
            .keyword = "required",
            .instance_location = "",
            .keyword_location = "/required",
            .absolute_keyword_location = "urn:webcpp:root#/required",
            .expected = &schema_value,
            .received = &instance_value(),
            .details = {.value = std::span<const std::string_view>(views)},
        };
        sink.fail(detail::error_access::make_error(fields));
        fields.kind = js::error_kind::unique_items;
        fields.details.value = std::span<const std::size_t>(indices);
        sink.fail(detail::error_access::make_error(fields));
        fields.kind = js::error_kind::max_items;
        fields.details.value = std::uint64_t{18446744073709551615ULL};
        sink.fail(detail::error_access::make_error(fields));
        fields.kind = js::error_kind::user;
        fields.details.value =
            boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
        sink.fail(detail::error_access::make_error(fields));
        for (std::string& name : names) {  // overwrite what the views pointed at
            name.assign(name.size(), 'x');
        }
        indices = {0, 0};
        sink.fail(e);
    }
    sink.leave(at({}), false);
    record.finish();
    if (!BOOST_TEST_EQ(record.errors().size(), 5U)) {
        return;
    }
    const auto* names =
        std::get_if<std::span<const std::string_view>>(&record.errors()[0].details().value);
    if (BOOST_TEST(names != nullptr) && BOOST_TEST_EQ(names->size(), 3U)) {
        BOOST_TEST_EQ((*names)[0], "first");
        BOOST_TEST_EQ((*names)[1], "a name long enough to leave the small buffer");
        BOOST_TEST_EQ((*names)[2], "");
    }
    BOOST_TEST(record.errors()[0].kind() == js::error_kind::required);
    BOOST_TEST_EQ(record.errors()[0].keyword(), "required");
    BOOST_TEST_EQ(record.errors()[0].absolute_keyword_location(), "urn:webcpp:root#/required");
    BOOST_TEST(&record.errors()[0].expected() == &schema_value);
    const auto* indices =
        std::get_if<std::span<const std::size_t>>(&record.errors()[1].details().value);
    if (BOOST_TEST(indices != nullptr) && BOOST_TEST_EQ(indices->size(), 2U)) {
        BOOST_TEST_EQ((*indices)[0], 3U);
        BOOST_TEST_EQ((*indices)[1], 5U);
    }
    const auto* count = std::get_if<std::uint64_t>(&record.errors()[2].details().value);
    BOOST_TEST(count != nullptr && *count == 18446744073709551615ULL);
    const auto* code = std::get_if<boost::system::error_code>(&record.errors()[3].details().value);
    BOOST_TEST(code != nullptr && *code == boost::system::errc::invalid_argument);
    BOOST_TEST(std::holds_alternative<std::monostate>(record.errors()[4].details().value));
    BOOST_TEST_EQ(record.errors()[4].instance_location(), "/name");
}

// An annotation's value is copied into the record's storage, so that one computed in a scratch
// storage survives it.
void annotation_values_are_copied_into_the_record() {
    std::pmr::monotonic_buffer_resource resource;
    detail::record record(resource, js::collect::errors_and_annotations);
    detail::collecting_sink sink(record);
    BOOST_TEST(record.storage().is_deallocate_trivial());
    sink.enter(at({}));
    {
        boost::json::monotonic_resource scratch;
        const boost::json::value computed = boost::json::parse(
            R"({"names": ["a", "a name long enough to leave the small buffer"], "deep": [[[1.5]]]})",
            &scratch);
        std::string location = "/properties";
        sink.annotate(make_annotation(location, &computed));
        location.assign(location.size(), 'x');
    }
    sink.leave(at({}), true);
    record.finish();
    if (!BOOST_TEST_EQ(record.annotations().size(), 1U)) {
        return;
    }
    const js::annotation& a = record.annotations()[0];
    BOOST_TEST_EQ(a.keyword_location(), "/properties");
    BOOST_TEST_EQ(a.keyword(), "title");
    BOOST_TEST_EQ(a.absolute_keyword_location(), "urn:webcpp:root#/title");
    BOOST_TEST_EQ(a.instance_location(), "");
    BOOST_TEST_EQ(
        boost::json::serialize(a.value()),
        R"({"names":["a","a name long enough to leave the small buffer"],"deep":[[[1.5E0]]]})");
    const boost::json::storage_ptr& storage = a.value().storage();
    BOOST_TEST(storage.get() == record.storage().get());
}

// The record and the sink allocate only through the given resource, and give back all of it.
void everything_lives_in_the_given_resource() {
    webcpp::test::counting_resource counted;
    std::pmr::memory_resource* const previous =
        std::pmr::set_default_resource(std::pmr::null_memory_resource());
    {
        detail::record record(counted, js::collect::everything);
        detail::collecting_sink sink(record);
        const boost::json::value value = boost::json::parse(R"(["a value", "with strings"])");
        const std::string name(80, 'p');
        const std::array<detail::path_token, 1> path{detail::path_token{.name = name}};
        for (int i = 0; i < 64; ++i) {
            sink.enter(at(path));
            sink.annotate(make_annotation(std::string(80, 'k'), &value));
            sink.fail(make_error(std::string(80, 'e')));
        }
        const auto mark = sink.checkpoint();
        sink.enter(at(path));
        sink.leave(at(path), false);
        sink.discard(mark);
        for (int i = 0; i < 64; ++i) {
            sink.leave(at(path), false);
        }
        record.finish();
        BOOST_TEST_EQ(record.recorded_annotations().size(), 64U);
        BOOST_TEST_EQ(record.errors().size(), 64U);
        BOOST_TEST(counted.allocations > 0);
    }
    std::pmr::set_default_resource(previous);
    BOOST_TEST_EQ(counted.live, 0U);
    BOOST_TEST_EQ(counted.allocations, counted.deallocations);
}

void finish_can_run_again() {
    std::pmr::monotonic_buffer_resource resource;
    detail::record record(resource, js::collect::everything);
    detail::collecting_sink sink(record);
    sink.enter(at({}));
    sink.annotate(make_annotation());
    sink.fail(make_error("/maxLength"));
    sink.leave(at({}), true);
    record.finish();
    record.finish();
    BOOST_TEST_EQ(record.errors().size(), 1U);
    BOOST_TEST_EQ(record.error_units().size(), 1U);
    BOOST_TEST_EQ(record.annotations().size(), 1U);
    BOOST_TEST_EQ(record.annotation_units().size(), 1U);
}

void the_flag_sink_records_nothing() {
    detail::flag_sink sink;
    const detail::event_context where = at({});
    const js::error e = make_error("/maxLength");
    const js::annotation a = make_annotation();
    static_assert(noexcept(sink.enter(where)));
    static_assert(noexcept(sink.leave(where, true)));
    static_assert(noexcept(sink.fail(e)));
    static_assert(noexcept(sink.annotate(a)));
    static_assert(noexcept(sink.checkpoint()));
    static_assert(noexcept(sink.discard(detail::flag_sink::mark{})));
    sink.enter(where);
    sink.fail(e);
    sink.annotate(a);
    sink.discard(sink.checkpoint());
    sink.leave(where, false);
    BOOST_TEST(!detail::flag_sink::records);
}

// The absolute location is a URI: its fragment is the node's pointer, percent-encoded, so that
// none of these names reads as another, as a%41 would read as aA.
void absolute_locations_are_uri_fragments() {
    const auto rows = std::to_array<std::pair<std::string_view, std::string_view>>({
        {"/properties/a b", "/properties/a%20b"},
        {"/properties/100%", "/properties/100%25"},
        {"/properties/a#b", "/properties/a%23b"},
        {"/properties/\xC3\xA9", "/properties/%C3%A9"},
        {"/properties/a\"b", "/properties/a%22b"},
        {"/properties/a%41", "/properties/a%2541"},
        {"/$defs/a~1b", "/$defs/a~1b"},
    });
    for (const auto& [pointer, encoded] : rows) {
        std::pmr::monotonic_buffer_resource resource;
        detail::record record(resource, js::collect::errors);
        detail::collecting_sink sink(record);
        const detail::event_context where{
            .evaluation_path = {},
            .instance_path = {},
            .resource_uri = "https://example.com/s.json",
            .schema_pointer = pointer,
        };
        sink.enter(where);
        sink.fail(make_error("/maxLength"));
        sink.leave(where, false);
        record.finish();
        if (BOOST_TEST_EQ(record.units().size(), 1U)) {
            BOOST_TEST_EQ(record.units()[0].absolute_keyword_location,
                          "https://example.com/s.json#" + std::string(encoded));
        }
    }
}

// Under any collect but everything, a node that ends valid with nothing recorded in it, its
// dropped descendants aside, leaves no unit; one that holds a counting annotation stays.
void valid_units_that_record_nothing_are_dropped() {
    for (const js::collect what :
         {js::collect::errors, js::collect::errors_and_annotations, js::collect::everything}) {
        std::pmr::monotonic_buffer_resource resource;
        detail::record record(resource, what);
        detail::collecting_sink sink(record);
        const std::array<detail::path_token, 1> a{detail::path_token{.name = "a"}};
        const std::array<detail::path_token, 1> b{detail::path_token{.name = "b"}};
        const std::array<detail::path_token, 1> r{detail::path_token{.name = "r"}};
        const std::array<detail::path_token, 1> c{detail::path_token{.name = "c"}};
        sink.enter(at({}));  // valid: kept only for what it holds
        sink.enter(at(a));   // valid, nothing recorded: dropped unless everything
        sink.enter(at(a));
        sink.leave(at(a), true);
        sink.leave(at(a), true);
        sink.enter(at(b));  // valid with an annotation: kept unless collect::errors
        sink.annotate(make_annotation("/b/title"));
        sink.leave(at(b), true);
        sink.leave(at({}), true);
        sink.enter(at(r));  // a second root, failed: kept
        sink.enter(at(c));
        sink.fail(make_error("/c/maxLength"));
        sink.leave(at(c), false);
        sink.leave(at(r), false);
        record.finish();
        std::vector<std::string_view> kept;
        for (const detail::unit& u : record.units()) {
            kept.push_back(u.keyword_location);
        }
        std::vector<std::string_view> expected{"", "/a", "/a", "/b", "/r", "/c"};
        if (what == js::collect::errors) {
            expected = {"/r", "/c"};
        } else if (what == js::collect::errors_and_annotations) {
            expected = {"", "/b", "/r", "/c"};
        }
        BOOST_TEST_ALL_EQ(kept.begin(), kept.end(), expected.begin(), expected.end());
        if (BOOST_TEST_EQ(record.error_units().size(), 1U)) {
            const detail::unit& failed = record.units()[record.error_units()[0]];
            BOOST_TEST_EQ(failed.keyword_location, "/c");
            BOOST_TEST_EQ(record.units()[failed.parent].keyword_location, "/r");
            BOOST_TEST_EQ(record.units()[failed.parent].parent, detail::no_unit);
        }
        if (what != js::collect::errors && BOOST_TEST_EQ(record.annotation_units().size(), 1U)) {
            BOOST_TEST_EQ(record.units()[record.annotation_units()[0]].keyword_location, "/b");
        }
    }
}

// The peak a valid walk takes from the given resource, `depth` levels deep, each level four
// keywords and one instance step deeper, under collect::errors.
std::size_t valid_walk_peak(std::size_t depth) {
    const std::vector<detail::path_token> keywords(4 * depth,
                                                   detail::path_token{.name = "properties"});
    const std::vector<detail::path_token> steps(depth, detail::path_token{.name = "member"});
    webcpp::test::counting_resource counted;
    {
        detail::record record(counted, js::collect::errors);
        detail::collecting_sink sink(record);
        const auto where = [&](std::size_t level) {
            return detail::event_context{
                .evaluation_path = std::span(keywords).first(4 * level),
                .instance_path = std::span(steps).first(level),
                .resource_uri = "urn:webcpp:root",
                .schema_pointer = "/properties/member",
            };
        };
        for (std::size_t level = 0; level < depth; ++level) {
            sink.enter(where(level));
        }
        for (std::size_t level = depth; level > 0; --level) {
            sink.leave(where(level - 1), true);
        }
        record.finish();
        BOOST_TEST_EQ(record.units().size(), 0U);
    }
    return counted.peak;
}

// A valid walk holds its open chain, not every location it passed: the peak grows linearly with
// the depth. Measured natively, 0.25 MiB at 1024 levels and 1.0 to 1.3 MiB at 4096; the arena's
// blocks grow geometrically, so the ratio steps around 4, where a walk that kept every location
// would take 16 times as much.
void a_valid_walk_takes_memory_linear_in_depth() {
    const std::size_t shallow = valid_walk_peak(1024);
    const std::size_t deep = valid_walk_peak(4096);
    BOOST_TEST_LE(shallow, std::size_t{1} << 20U);
    BOOST_TEST_LE(deep, std::size_t{4} << 20U);
    BOOST_TEST_LE(deep, 8 * shallow);
}

// Copying an annotation's value gives the copy's work stacks back to the given resource, so that
// none stays in the record's arena.
void annotation_copies_give_their_scratch_back() {
    webcpp::test::counting_resource counted;
    detail::record record(counted, js::collect::errors_and_annotations);
    detail::collecting_sink sink(record);
    const boost::json::value value = boost::json::parse(R"({"a": [1, 2, {"b": null}]})");
    sink.enter(at({}));
    const std::size_t before = counted.deallocations;
    constexpr std::size_t copies = 100;
    for (std::size_t i = 0; i < copies; ++i) {
        sink.annotate(make_annotation("/title", &value));
    }
    BOOST_TEST_GE(counted.deallocations - before, copies);
    sink.leave(at({}), true);
    record.finish();
    BOOST_TEST_EQ(record.annotations().size(), copies);
}

}  // namespace

int main() {
    error_kinds_are_fixed();
    every_error_kind_has_its_value();
    errors_are_copied_and_located();
    a_failed_node_drops_its_annotations_and_keeps_its_errors();
    discard_truncates_unless_everything_is_collected();
    collect_errors_records_no_annotation();
    locations_are_json_pointers();
    units_have_parents_and_results();
    a_failed_node_drops_its_descendants_annotations();
    discards_nest_under_every_collect();
    deep_nesting_keeps_p2();
    details_are_copied();
    annotation_values_are_copied_into_the_record();
    everything_lives_in_the_given_resource();
    finish_can_run_again();
    the_flag_sink_records_nothing();
    absolute_locations_are_uri_fragments();
    valid_units_that_record_nothing_are_dropped();
    a_valid_walk_takes_memory_linear_in_depth();
    annotation_copies_give_their_scratch_back();
    return boost::report_errors();
}
