// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/errors.hpp>

#include <boost/core/lightweight_test.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <boost/json/value.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace js = webcpp::json_schema;

namespace {

void values_are_fixed() {
    constexpr std::array<js::errc, 20> all{
        js::errc::not_a_schema,
        js::errc::invalid_keyword_value,
        js::errc::meta_schema_violation,
        js::errc::unknown_dialect,
        js::errc::unknown_vocabulary,
        js::errc::unknown_keyword,
        js::errc::invalid_uri,
        js::errc::unresolvable_reference,
        js::errc::resolver_failed,
        js::errc::invalid_json_pointer,
        js::errc::duplicate_resource,
        js::errc::duplicate_anchor,
        js::errc::reference_cycle,
        js::errc::invalid_regex,
        js::errc::schema_too_deep,
        js::errc::invalid_number,
        js::errc::invalid_ir,
        js::errc::unrepresentable,
        js::errc::output_not_collected,
        js::errc::vocabulary_conflict,
    };
    for (std::size_t i = 0; i < all.size(); ++i) {
        BOOST_TEST_EQ(static_cast<int>(all[i]), static_cast<int>(i) + 1);
    }
}

void the_category_is_webcpp_json_schema() {
    BOOST_TEST_EQ(std::string(js::error_category().name()), "webcpp.json_schema");
    const boost::system::error_code code = js::errc::reference_cycle;
    BOOST_TEST(code.category() == js::error_category());
    BOOST_TEST_EQ(code.message(), "a cycle of references consumes no instance");
    BOOST_TEST_EQ(js::make_error_code(js::errc::not_a_schema).message(),
                  "the value is not a schema");
    BOOST_TEST_EQ(js::make_error_code(js::errc::vocabulary_conflict).message(),
                  "a user keyword's name is taken");
    BOOST_TEST_EQ(js::error_category().message(0), "unknown webcpp.json_schema error");
    BOOST_TEST_EQ(js::error_category().message(21), "unknown webcpp.json_schema error");
    std::array<char, 8> small{};
    BOOST_TEST_EQ(std::string(js::error_category().message(1, small.data(), small.size())),
                  "the value is not a schema");
    BOOST_TEST_EQ(std::string(small.data()), "the val");
}

void a_diagnostic_names_its_place_without_allocating() {
    const std::string long_text(300, 'x');
    const js::diagnostic d =
        js::detail::make_diagnostic(js::errc::unknown_dialect, long_text, "draft-03");
    BOOST_TEST(d.code == js::errc::unknown_dialect);
    BOOST_TEST_EQ(d.where.size(), 256U);
    BOOST_TEST_EQ(std::string(d.where.substr(253)), "...");
    BOOST_TEST_EQ(std::string(d.what), "draft-03");
}

// Whether `text` is well-formed UTF-8 as far as its sequences' lengths go: each lead byte is
// followed by as many continuation bytes as it announces, and the text ends after the last.
bool is_utf8(std::string_view text) {
    std::size_t i = 0;
    while (i < text.size()) {
        const auto lead = static_cast<unsigned char>(text[i]);
        std::size_t length = 0;
        if (lead < 0x80U) {
            length = 1;
        } else if ((lead & 0xE0U) == 0xC0U) {
            length = 2;
        } else if ((lead & 0xF0U) == 0xE0U) {
            length = 3;
        } else if ((lead & 0xF8U) == 0xF0U) {
            length = 4;
        } else {
            return false;
        }
        if (length > text.size() - i) {
            return false;
        }
        for (std::size_t k = 1; k < length; ++k) {
            if ((static_cast<unsigned char>(text[i + k]) & 0xC0U) != 0x80U) {
                return false;
            }
        }
        i += length;
    }
    return true;
}

// Whether `text`, written by Boost.JSON as a JSON string, parses back to itself.
bool round_trips_as_json(std::string_view text) {
    const std::string written = boost::json::serialize(boost::json::value(text));
    boost::system::error_code ec;
    const boost::json::value read = boost::json::parse(written, ec);
    return !ec && read.is_string() && std::string_view(read.get_string()) == text;
}

// A text that the diagnostic holds, and what it holds of it.
struct cut_case {
    std::string text;
    std::string expected;
};

// The cut keeps at most 256 bytes and never splits a UTF-8 sequence: it steps back over the
// continuation bytes at the cut, so a sequence that straddles it goes whole, before the "...".
void a_cut_keeps_whole_code_points() {
    const std::string euro = "\xE2\x82\xAC";
    const std::string e_acute = "\xC3\xA9";
    const std::string grin = "\xF0\x9F\x98\x80";
    const std::array<cut_case, 11> cases{
        // 256 bytes are kept whole; 257 are cut to 253 and "...".
        cut_case{.text = std::string(256, 'x'), .expected = std::string(256, 'x')},
        cut_case{.text = std::string(257, 'y'), .expected = std::string(253, 'y') + "..."},
        // A 2-byte sequence at bytes 252 and 253.
        cut_case{
            .text = std::string(252, 'a') + e_acute + std::string(10, 'b'),
            .expected = std::string(252, 'a') + "...",
        },
        // A 3-byte sequence at bytes 251 to 253, and at 252 to 254.
        cut_case{
            .text = std::string(251, 'a') + euro + std::string(10, 'b'),
            .expected = std::string(251, 'a') + "...",
        },
        cut_case{
            .text = std::string(252, 'a') + euro + std::string(50, 'b'),
            .expected = std::string(252, 'a') + "...",
        },
        // A 4-byte sequence at bytes 250 to 253, 251 to 254, and from 253, the first byte cut.
        cut_case{
            .text = std::string(250, 'a') + grin + std::string(10, 'b'),
            .expected = std::string(250, 'a') + "...",
        },
        cut_case{
            .text = std::string(251, 'a') + grin + std::string(10, 'b'),
            .expected = std::string(251, 'a') + "...",
        },
        cut_case{
            .text = std::string(253, 'a') + grin + std::string(10, 'b'),
            .expected = std::string(253, 'a') + "...",
        },
        // A 4-byte sequence that ends at byte 252, just before the cut, is kept.
        cut_case{
            .text = std::string(249, 'a') + grin + std::string(10, 'b'),
            .expected = std::string(249, 'a') + grin + "...",
        },
        // A 2-byte sequence at bytes 251 and 252, which ends at the cut, is kept.
        cut_case{
            .text = std::string(251, 'a') + e_acute + std::string(10, 'b'),
            .expected = std::string(251, 'a') + e_acute + "...",
        },
        // 256 bytes that end with a 4-byte sequence are kept whole.
        cut_case{.text = std::string(252, 'a') + grin, .expected = std::string(252, 'a') + grin},
    };
    for (const cut_case& c : cases) {
        const js::diagnostic d = js::detail::make_diagnostic(js::errc::invalid_uri, c.text, c.text);
        BOOST_TEST_EQ(std::string(d.where), c.expected);
        BOOST_TEST_EQ(std::string(d.what), c.expected);
        BOOST_TEST_LE(d.where.size(), 256U);
        BOOST_TEST(is_utf8(std::string_view(d.where)));
        BOOST_TEST(round_trips_as_json(std::string_view(d.where)));
    }
}

// Malformed text: the cut moves back only before a lead byte whose sequence it would split, at
// most three bytes, so a truncated sequence at the cut goes and orphan continuation bytes stay.
void a_cut_of_malformed_text_moves_back_three_bytes_at_most() {
    const std::string orphans(10, '\x80');
    const std::string euro = "\xE2\x82\xAC";
    const std::array<cut_case, 5> cases{
        // Continuation bytes with no lead byte, from byte 250: cut at byte 253.
        cut_case{
            .text = std::string(250, 'a') + orphans + std::string(10, 'b'),
            .expected = std::string(250, 'a') + orphans.substr(0, 3) + "...",
        },
        // A whole sequence at bytes 248 to 250, then orphans: both kept up to byte 253.
        cut_case{
            .text = std::string(248, 'a') + euro + orphans,
            .expected = std::string(248, 'a') + euro + orphans.substr(0, 2) + "...",
        },
        // A 3-byte sequence from byte 251, truncated at byte 253 by a "b": it goes whole.
        cut_case{
            .text = std::string(251, 'a') + "\xE2\x82" + std::string(10, 'b'),
            .expected = std::string(251, 'a') + "...",
        },
        // A lead byte at byte 251 that its sequence lacks, then a 3-byte sequence across the cut:
        // the cut moves before the first.
        cut_case{
            .text = std::string(251, 'a') + "\xE2" + euro + std::string(10, 'b'),
            .expected = std::string(251, 'a') + "...",
        },
        // A 4-byte sequence from byte 250, truncated at byte 253: it goes whole.
        cut_case{
            .text = std::string(250, 'a') + "\xF0\x9F\x98" + std::string(10, 'b'),
            .expected = std::string(250, 'a') + "...",
        },
    };
    for (const cut_case& c : cases) {
        const js::diagnostic d = js::detail::make_diagnostic(js::errc::invalid_uri, c.text, c.text);
        BOOST_TEST_EQ(std::string(d.where), c.expected);
        BOOST_TEST_EQ(std::string(d.what), c.expected);
        BOOST_TEST_EQ(d.where.size(), c.expected.size());
    }
}

void a_result_holds_a_diagnostic() {
    const js::result<int> r = js::detail::make_diagnostic(js::errc::invalid_uri, "/$id", "a b");
    BOOST_TEST(r.has_error());
    BOOST_TEST(r.error().code == js::errc::invalid_uri);
    BOOST_TEST_EQ(std::string(r.error().where), "/$id");
}

}  // namespace

int main() {
    values_are_fixed();
    the_category_is_webcpp_json_schema();
    a_diagnostic_names_its_place_without_allocating();
    a_cut_keeps_whole_code_points();
    a_cut_of_malformed_text_moves_back_three_bytes_at_most();
    a_result_holds_a_diagnostic();
    return boost::report_errors();
}
