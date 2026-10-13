// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

// The traits: the default ones model the concept, a number type, an allocator or a regex policy
// that is not one is refused; the dialects: their fixed values, the tags, the tag of a dialect
// json_schema loads a dialect policy from this header alone and a tag of none not one, each
// official URI read with and without its empty fragment and spelled as the meta-schemas spell it,
// and nothing else read.

#include <webcpp/json_schema/dialects/dialect.hpp>
#include <webcpp/json_schema/traits.hpp>

#include "recording_resource.hpp"

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>
#include <type_traits>

namespace js = webcpp::json_schema;

static_assert(js::traits<js::default_traits>);
static_assert(js::dialect_policy<js::draft2020_12>);
static_assert(std::is_same_v<js::basic_string<js::default_traits>, std::pmr::string>);

namespace {

struct recording_traits : js::default_traits {
    using allocator_type = webcpp::test::recording_allocator<std::byte>;
};

struct char_allocator_traits : js::default_traits {
    using allocator_type = std::pmr::polymorphic_allocator<char>;
};

struct double_traits : js::default_traits {
    using number_type = double;
};

struct no_regex_traits : js::default_traits {
    struct regex_policy {};
};

struct not_a_dialect {};

// A tag with an id and no machinery: a dialect json_schema does not load.
struct without_machinery {
    static constexpr js::dialect id = js::dialect::draft4;
};

}  // namespace

static_assert(js::traits<recording_traits>);
static_assert(std::is_same_v<js::basic_string<recording_traits>,
                             std::basic_string<char, std::char_traits<char>,
                                               webcpp::test::recording_allocator<char>>>);
static_assert(!js::traits<char_allocator_traits>);
static_assert(!js::traits<double_traits>);
static_assert(!js::traits<no_regex_traits>);

static_assert(!js::dialect_policy<not_a_dialect>);
static_assert(!js::dialect_policy<without_machinery>);
static_assert(js::draft4::id == js::dialect::draft4);
static_assert(js::draft6::id == js::dialect::draft6);
static_assert(js::draft7::id == js::dialect::draft7);
static_assert(js::draft2019_09::id == js::dialect::draft2019_09);
static_assert(js::draft2020_12::id == js::dialect::draft2020_12);
static_assert(js::openapi3_0::id == js::dialect::openapi3_0);
static_assert(std::is_same_v<std::underlying_type_t<js::dialect>, std::uint8_t>);

// Both are usable in a constant expression.
static_assert(js::uri_of(js::dialect::draft6) == "http://json-schema.org/draft-06/schema#");
static_assert(js::dialect_of_uri("http://json-schema.org/draft-04/schema") == js::dialect::draft4);

namespace {

void values_and_spellings() {
    BOOST_TEST_EQ(static_cast<int>(js::dialect::draft4), 1);
    BOOST_TEST_EQ(static_cast<int>(js::dialect::draft6), 2);
    BOOST_TEST_EQ(static_cast<int>(js::dialect::draft7), 3);
    BOOST_TEST_EQ(static_cast<int>(js::dialect::draft2019_09), 4);
    BOOST_TEST_EQ(static_cast<int>(js::dialect::draft2020_12), 5);
    BOOST_TEST_EQ(static_cast<int>(js::dialect::openapi3_0), 6);
    BOOST_TEST_EQ(js::uri_of(js::dialect::draft2020_12),
                  "https://json-schema.org/draft/2020-12/schema");
    BOOST_TEST_EQ(js::uri_of(js::dialect::draft7), "http://json-schema.org/draft-07/schema#");
    BOOST_TEST_EQ(js::uri_of(js::dialect::openapi3_0), "");
    BOOST_TEST(js::dialect_of_uri("https://json-schema.org/draft/2020-12/schema#") ==
               js::dialect::draft2020_12);
    BOOST_TEST(js::dialect_of_uri("http://json-schema.org/draft-07/schema") == js::dialect::draft7);
    BOOST_TEST(!js::dialect_of_uri("https://json-schema.org/draft-07/schema#"));
    BOOST_TEST(!js::dialect_of_uri("https://json-schema.org/draft/2019-09/schema/"));
    BOOST_TEST(!js::dialect_of_uri("http://json-schema.org/draft-03/schema#"));
    BOOST_TEST(!js::dialect_of_uri(""));
}

struct official {
    js::dialect id;
    std::string_view with_fragment;
    std::string_view without_fragment;
    std::string_view spelled;  // as the meta-schema's own "$schema" and "id"/"$id" spell it
};

// The spellings, copied from the meta-schemas json-schema.org publishes, not from the header.
constexpr std::array officials{
    official{
        .id = js::dialect::draft4,
        .with_fragment = "http://json-schema.org/draft-04/schema#",
        .without_fragment = "http://json-schema.org/draft-04/schema",
        .spelled = "http://json-schema.org/draft-04/schema#",
    },
    official{
        .id = js::dialect::draft6,
        .with_fragment = "http://json-schema.org/draft-06/schema#",
        .without_fragment = "http://json-schema.org/draft-06/schema",
        .spelled = "http://json-schema.org/draft-06/schema#",
    },
    official{
        .id = js::dialect::draft7,
        .with_fragment = "http://json-schema.org/draft-07/schema#",
        .without_fragment = "http://json-schema.org/draft-07/schema",
        .spelled = "http://json-schema.org/draft-07/schema#",
    },
    official{
        .id = js::dialect::draft2019_09,
        .with_fragment = "https://json-schema.org/draft/2019-09/schema#",
        .without_fragment = "https://json-schema.org/draft/2019-09/schema",
        .spelled = "https://json-schema.org/draft/2019-09/schema",
    },
    official{
        .id = js::dialect::draft2020_12,
        .with_fragment = "https://json-schema.org/draft/2020-12/schema#",
        .without_fragment = "https://json-schema.org/draft/2020-12/schema",
        .spelled = "https://json-schema.org/draft/2020-12/schema",
    },
};

void every_official_uri_both_ways() {
    for (const official& o : officials) {
        BOOST_TEST(js::dialect_of_uri(o.with_fragment) == o.id);
        BOOST_TEST(js::dialect_of_uri(o.without_fragment) == o.id);
        BOOST_TEST_EQ(js::uri_of(o.id), o.spelled);
        BOOST_TEST(js::dialect_of_uri(js::uri_of(o.id)) == o.id);
    }
}

void nothing_else_is_read() {
    for (const official& o : officials) {
        const std::string uri(o.without_fragment);
        BOOST_TEST(!js::dialect_of_uri(uri + "##"));
        BOOST_TEST(!js::dialect_of_uri(uri + "#/"));
        BOOST_TEST(!js::dialect_of_uri(uri + "#meta"));
        BOOST_TEST(!js::dialect_of_uri(uri + "/"));
        BOOST_TEST(!js::dialect_of_uri(uri + " "));
        BOOST_TEST(!js::dialect_of_uri(" " + uri));
        BOOST_TEST(!js::dialect_of_uri(uri + "?"));
        BOOST_TEST(!js::dialect_of_uri(uri.substr(0, uri.size() - 1)));
        std::string upper = uri;
        upper[0] = 'H';
        BOOST_TEST(!js::dialect_of_uri(upper));
    }
    BOOST_TEST(!js::dialect_of_uri("#"));
    BOOST_TEST(!js::dialect_of_uri("http://json-schema.org/schema#"));
    BOOST_TEST(!js::dialect_of_uri("https://json-schema.org/draft/2020-12/meta/core"));
    BOOST_TEST(!js::dialect_of_uri("https://spec.openapis.org/oas/3.0/schema/2021-09-28"));
    // a NUL byte is a byte like any other: the URI it ends is not the official one
    const std::string with_nul("http://json-schema.org/draft-07/schema\0", 39);
    BOOST_TEST(!js::dialect_of_uri(with_nul));
}

}  // namespace

int main() {
    values_and_spellings();
    every_official_uri_both_ways();
    nothing_else_is_read();
    return boost::report_errors();
}
