// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/output/message_hook.hpp>
#include <webcpp/json_schema/validator/options.hpp>

#include "recording_resource.hpp"

#include <boost/compat/function_ref.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/json/monotonic_resource.hpp>
#include <boost/json/storage_ptr.hpp>
#include <boost/json/value.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/result.hpp>

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace js = webcpp::json_schema;

namespace {

namespace detail = js::detail;

struct recording_traits : js::default_traits {
    using allocator_type = webcpp::test::recording_allocator<std::byte>;
};

// The options are aggregates a program designates, and the data types of the later features too.
static_assert(std::is_aggregate_v<js::compile_options>);
static_assert(std::is_aggregate_v<js::validate_options>);
static_assert(std::is_aggregate_v<js::format_entry>);
static_assert(std::is_aggregate_v<js::user_call>);
static_assert(std::is_aggregate_v<js::basic_user_keyword<js::default_traits>>);
static_assert(std::is_aggregate_v<js::basic_vocabulary<js::default_traits>>);
static_assert(std::is_same_v<js::format_set, std::span<const js::format_entry>>);
static_assert(std::is_same_v<js::registry, js::basic_registry<js::default_traits>>);
static_assert(std::is_same_v<js::compile_options, js::basic_compile_options<js::default_traits>>);
static_assert(std::is_same_v<js::validate_options, js::basic_validate_options<js::default_traits>>);
static_assert(std::is_same_v<js::resolver_ref,
                             boost::compat::function_ref<boost::system::result<boost::json::value>(
                                 std::string_view, const boost::json::storage_ptr&)>>);
static_assert(
    std::is_same_v<
        js::basic_message_hook<recording_traits>,
        boost::compat::function_ref<void(const js::error&, js::basic_string<recording_traits>&)>>);
static_assert(
    std::is_nothrow_invocable_r_v<bool, decltype(js::format_entry::check), std::string_view>);
static_assert(js::default_max_schema_depth == 1024 && js::default_max_depth == 1024);
static_assert(std::is_same_v<std::underlying_type_t<js::unknown_keywords>, std::uint8_t>);
static_assert(std::is_same_v<std::underlying_type_t<js::activation>, std::uint8_t>);

// A registry made from an allocator that has no default has no default constructor.
static_assert(std::default_initializable<js::registry>);
static_assert(!std::default_initializable<js::basic_registry<recording_traits>>);
static_assert(std::is_nothrow_constructible_v<js::registry, js::registry::allocator_type>);

// A registry moves, taking the source's allocator, and is never copied, as a document is.
static_assert(!std::is_copy_constructible_v<js::registry>);
static_assert(!std::is_copy_assignable_v<js::registry>);
static_assert(std::is_nothrow_move_constructible_v<js::registry>);
static_assert(std::is_nothrow_move_assignable_v<js::registry>);
static_assert(std::is_final_v<js::registry>);
static_assert(std::is_nothrow_move_assignable_v<js::basic_registry<recording_traits>>);

const boost::json::value schema_value = 3;
const boost::json::value instance_value = 4;

js::error make_error() {
    return detail::error_access::make_error({
        .kind = js::error_kind::maximum,
        .keyword = "maximum",
        .instance_location = "",
        .keyword_location = "/maximum",
        .absolute_keyword_location = "urn:webcpp:root#/maximum",
        .expected = &schema_value,
        .received = &instance_value,
    });
}

void the_defaults_are_the_specifications() {
    const js::compile_options compile{
        .default_dialect = js::dialect::draft2020_12,
        .validate_schema = false,
    };
    BOOST_TEST(compile.registry == nullptr);
    BOOST_TEST(!compile.resolver);
    BOOST_TEST(compile.base_uri.empty());
    BOOST_TEST(!compile.format_assertion);
    BOOST_TEST(compile.formats.empty());
    BOOST_TEST_EQ(compile.regex_step_budget, 0U);
    BOOST_TEST(compile.unknown == js::unknown_keywords::dialect_default);
    BOOST_TEST(compile.compatibility_keywords);
    BOOST_TEST(!compile.validate_schema);
    BOOST_TEST_EQ(compile.max_schema_depth, js::default_max_schema_depth);
    const js::validate_options validate{.what = js::collect::everything};
    BOOST_TEST(!validate.messages);
    BOOST_TEST_EQ(validate.max_depth, js::default_max_depth);
    static_assert(
        std::is_same_v<js::basic_message_hook<js::default_traits>,
                       boost::compat::function_ref<void(const js::error&, std::pmr::string&)>>);
}

void the_untouched_defaults() {
    const js::compile_options compile{};
    BOOST_TEST(!compile.default_dialect);
    BOOST_TEST(!compile.content_assertion);
    BOOST_TEST(compile.vocabularies.empty());
    BOOST_TEST(compile.validate_schema);
    const js::validate_options validate{};
    BOOST_TEST(validate.what == js::collect::errors);
}

// Every member designated, in the specification's order, which a designated initializer requires.
void the_members_are_in_the_specifications_order() {
    js::registry registry;
    auto resolve = [](std::string_view, const boost::json::storage_ptr&) {
        return boost::system::result<boost::json::value>(nullptr);
    };
    const js::compile_options compile{
        .registry = &registry,
        .resolver = resolve,
        .base_uri = "https://example.com/root.json",
        .default_dialect = js::dialect::draft7,
        .format_assertion = true,
        .formats = {},
        .content_assertion = true,
        .regex_step_budget = 1000,
        .unknown = js::unknown_keywords::reject,
        .compatibility_keywords = false,
        .validate_schema = false,
        .vocabularies = {},
        .max_schema_depth = 64,
    };
    BOOST_TEST(compile.registry == &registry);
    BOOST_TEST(compile.resolver.has_value());
    BOOST_TEST_EQ(compile.base_uri, "https://example.com/root.json");
    BOOST_TEST(compile.default_dialect == js::dialect::draft7);
    BOOST_TEST_EQ(compile.regex_step_budget, 1000U);
    BOOST_TEST(compile.unknown == js::unknown_keywords::reject);
    BOOST_TEST_EQ(compile.max_schema_depth, 64U);

    auto write = [](const js::error&, std::pmr::string& out) { out += "no"; };
    const js::validate_options validate{
        .messages = write,
        .what = js::collect::errors_and_annotations,
        .max_depth = 32,
    };
    BOOST_TEST(validate.messages.has_value());
    BOOST_TEST(validate.what == js::collect::errors_and_annotations);
    BOOST_TEST_EQ(validate.max_depth, 32U);
}

void a_registry_keeps_its_allocator() {
    webcpp::test::recording_resource resource;
    const js::registry registry(&resource);
    BOOST_TEST(registry.get_allocator().resource() == &resource);
    BOOST_TEST_EQ(resource.allocations, 0U);

    const webcpp::test::recording_allocator<std::byte> allocator(&resource);
    const js::basic_registry<recording_traits> recording(allocator);
    BOOST_TEST(recording.get_allocator() == allocator);
    BOOST_TEST_EQ(resource.allocations, 0U);

    const js::registry by_default;
    BOOST_TEST(by_default.get_allocator().resource() == std::pmr::get_default_resource());
}

// A moved-to registry holds the source's allocator, whatever allocator it held before.
void a_moved_registry_takes_the_sources_allocator() {
    webcpp::test::recording_resource source_resource;
    webcpp::test::recording_resource target_resource;
    js::registry source(&source_resource);
    const js::registry constructed(std::move(source));
    BOOST_TEST(constructed.get_allocator().resource() == &source_resource);

    js::registry other(&source_resource);
    js::registry assigned(&target_resource);
    assigned = std::move(other);
    BOOST_TEST(assigned.get_allocator().resource() == &source_resource);

    const webcpp::test::recording_allocator<std::byte> from(&source_resource);
    const webcpp::test::recording_allocator<std::byte> to(&target_resource);
    js::basic_registry<recording_traits> recording_source(from);
    js::basic_registry<recording_traits> recording_target(to);
    recording_target = std::move(recording_source);
    BOOST_TEST(recording_target.get_allocator() == from);
    BOOST_TEST_EQ(source_resource.allocations + target_resource.allocations, 0U);
}

// The hook writes into the string it is given, so its text lands in that string's allocator.
void the_hook_writes_into_the_validations_allocator() {
    webcpp::test::recording_resource resource;
    auto write = [](const js::error& e, std::pmr::string& out) {
        out += "The value at '";
        out += e.instance_location();
        out += "' is above the bound of '";
        out += e.keyword();
        out += "', which this sentence states at length.";
    };
    const js::validate_options options{.messages = write};
    std::pmr::string message(&resource);
    const js::error e = make_error();
    BOOST_TEST(options.messages.has_value());
    if (options.messages) {
        (*options.messages)(e, message);
    }
    BOOST_TEST_EQ(message,
                  "The value at '' is above the bound of 'maximum', which this sentence states at "
                  "length.");
    BOOST_TEST_GE(resource.allocations, 1U);
}

// The resolver builds its value in the storage the compile gives it.
void the_resolver_builds_in_the_storage_given() {
    auto resolve = [](std::string_view uri, const boost::json::storage_ptr& storage) {
        using answer = boost::system::result<boost::json::value>;
        if (uri != "https://example.com/known.json") {
            return answer(boost::system::errc::make_error_code(
                boost::system::errc::no_such_file_or_directory));
        }
        return answer(boost::json::value({{"type", "string"}, {"minLength", 1}}, storage));
    };
    const js::compile_options options{.resolver = resolve};
    boost::json::monotonic_resource arena;
    const boost::json::storage_ptr storage(&arena);
    BOOST_TEST(options.resolver.has_value());
    if (!options.resolver) {
        return;
    }
    const auto known = (*options.resolver)("https://example.com/known.json", storage);
    BOOST_TEST(known.has_value());
    if (known) {
        BOOST_TEST(known->storage().get() == &arena);
        BOOST_TEST(known->is_object());
    }
    BOOST_TEST(!(*options.resolver)("https://example.com/unknown.json", storage));
}

// A format set is the user's array; each check is noexcept.
void a_format_set_is_the_users_array() {
    auto is_upper = [](std::string_view text) noexcept {
        return std::ranges::all_of(text, [](char c) { return c >= 'A' && c <= 'Z'; });
    };
    auto is_empty = [](std::string_view text) noexcept { return text.empty(); };
    const std::array entries{
        js::format_entry{.name = "upper", .check = is_upper},
        js::format_entry{.name = "empty", .check = is_empty},
    };
    const js::compile_options options{.format_assertion = true, .formats = entries};
    BOOST_TEST_EQ(options.formats.size(), 2U);
    BOOST_TEST_EQ(options.formats[0].name, "upper");
    BOOST_TEST(options.formats[0].check("ABC"));
    BOOST_TEST(!options.formats[0].check("AbC"));
    BOOST_TEST(options.formats[1].check(""));
    BOOST_TEST(!options.formats[1].check("x"));
}

// A user keyword: its defaults, its functions, and its vocabulary's.
void a_user_vocabulary_holds_its_keywords() {
    auto check = [](const boost::json::value& value) -> boost::system::result<void> {
        if (!value.is_int64()) {
            return boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
        }
        return {};
    };
    auto validate = [](const js::user_call& call) -> boost::system::result<bool> {
        if (!call.instance.is_string()) {
            return true;
        }
        const auto size = static_cast<std::int64_t>(call.instance.get_string().size());
        return size % call.value.as_int64() == 0;
    };
    const js::basic_user_keyword<js::default_traits> keyword{
        .name = "lengthMultipleOf",
        .check = check,
        .validate = validate,
    };
    const std::array keywords{keyword};
    BOOST_TEST(keyword.applies_to.bits == js::ir::type_set::all().bits);
    BOOST_TEST(!keyword.annotates);
    BOOST_TEST(!keyword.write);
    BOOST_TEST(keyword.check(boost::json::value(2)).has_value());
    BOOST_TEST(!keyword.check(boost::json::value("two")).has_value());
    const boost::json::value divisor = 2;
    const boost::json::value even = "ab";
    const boost::json::value odd = "abc";
    const auto passes = keyword.validate({.value = divisor, .instance = even});
    BOOST_TEST(passes.has_value() && *passes);
    const auto fails = keyword.validate({.value = divisor, .instance = odd});
    BOOST_TEST(fails.has_value() && !*fails);

    const js::basic_vocabulary<js::default_traits> vocabulary{
        .uri = "https://example.com/vocab/lengths",
        .keywords = keywords,
    };
    const std::array vocabularies{vocabulary};
    BOOST_TEST(vocabularies[0].active == js::activation::always);
    const js::compile_options options{.vocabularies = vocabularies};
    BOOST_TEST_EQ(options.vocabularies.size(), 1U);
    BOOST_TEST_EQ(options.vocabularies[0].keywords[0].name, "lengthMultipleOf");
}

}  // namespace

int main() {
    the_defaults_are_the_specifications();
    the_untouched_defaults();
    the_members_are_in_the_specifications_order();
    a_registry_keeps_its_allocator();
    a_moved_registry_takes_the_sources_allocator();
    the_hook_writes_into_the_validations_allocator();
    the_resolver_builds_in_the_storage_given();
    a_format_set_is_the_users_array();
    a_user_vocabulary_holds_its_keywords();
    return boost::report_errors();
}
