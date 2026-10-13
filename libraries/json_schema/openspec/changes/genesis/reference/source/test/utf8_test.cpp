// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

#include <webcpp/json_schema/detail/json/utf8.hpp>

#include <boost/core/lightweight_test.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>

namespace js = webcpp::json_schema;

namespace {

// The oracle below reads UTF-8 by its definition (RFC 3629 section 3), not by its table of
// sequences: a sequence's bits give a number, and the sequence is valid when that number is a
// scalar value (at most U+10FFFF, not a surrogate) whose shortest encoding is the sequence itself.

struct loose_sequence {
    std::uint32_t value;
    std::size_t length;
};

// One sequence read by its bit pattern alone, overlong forms, surrogates and numbers past U+10FFFF
// included; nothing when a lead byte or a continuation byte has the wrong pattern, or bytes lack.
std::optional<loose_sequence> read_loose(std::string_view s) {
    const auto lead = static_cast<unsigned char>(s.front());
    std::size_t length = 0;
    std::uint32_t value = 0;
    if ((lead & 0x80U) == 0) {
        return loose_sequence{.value = lead, .length = 1};
    }
    if ((lead & 0xE0U) == 0xC0U) {
        length = 2;
        value = lead & 0x1FU;
    } else if ((lead & 0xF0U) == 0xE0U) {
        length = 3;
        value = lead & 0x0FU;
    } else if ((lead & 0xF8U) == 0xF0U) {
        length = 4;
        value = lead & 0x07U;
    } else {
        return std::nullopt;
    }
    if (s.size() < length) {
        return std::nullopt;
    }
    for (std::size_t k = 1; k < length; ++k) {
        const auto c = static_cast<unsigned char>(s[k]);
        if ((c & 0xC0U) != 0x80U) {
            return std::nullopt;
        }
        value = (value << 6U) | (c & 0x3FU);
    }
    return loose_sequence{.value = value, .length = length};
}

// The shortest encoding of any number below 2^21, a surrogate's too.
std::string encode(std::uint32_t value) {
    std::string out;
    if (value < 0x80U) {
        out.push_back(static_cast<char>(value));
    } else if (value < 0x800U) {
        out.push_back(static_cast<char>(0xC0U | (value >> 6U)));
        out.push_back(static_cast<char>(0x80U | (value & 0x3FU)));
    } else if (value < 0x10000U) {
        out.push_back(static_cast<char>(0xE0U | (value >> 12U)));
        out.push_back(static_cast<char>(0x80U | ((value >> 6U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (value & 0x3FU)));
    } else {
        out.push_back(static_cast<char>(0xF0U | (value >> 18U)));
        out.push_back(static_cast<char>(0x80U | ((value >> 12U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | ((value >> 6U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (value & 0x3FU)));
    }
    return out;
}

bool is_scalar(std::uint32_t value) {
    return value <= 0x10FFFFU && (value < 0xD800U || value > 0xDFFFU);
}

// The oracle: whether `s` is valid UTF-8, and its count of code points when it is.
std::optional<std::size_t> oracle(std::string_view s) {
    std::size_t count = 0;
    while (!s.empty()) {
        const auto sequence = read_loose(s);
        if (!sequence || !is_scalar(sequence->value) ||
            encode(sequence->value) != s.substr(0, sequence->length)) {
            return std::nullopt;
        }
        s.remove_prefix(sequence->length);
        ++count;
    }
    return count;
}

// The string of `values`, each a byte.
std::string bytes(std::initializer_list<unsigned> values) {
    std::string out;
    for (const unsigned value : values) {
        out.push_back(static_cast<char>(value));
    }
    return out;
}

// Both functions agree with the oracle on `s`: alone; between two ASCII bytes, so that a sequence
// is also read away from the string's start and end; and as a view whose next bytes in memory are
// continuation bytes, so that a sequence cut by the view's end is not completed by what follows.
void agrees(const std::string& s) {
    const std::string followed = s + "\x80\xBF\x80";
    const std::string between = "a" + s + "b";
    const std::string_view cut = std::string_view(followed).substr(0, s.size());
    for (const std::string_view text : {std::string_view(s), std::string_view(between), cut}) {
        const auto expected = oracle(text);
        if (!BOOST_TEST_EQ(js::detail::is_valid_utf8(text), expected.has_value())) {
            return;
        }
        if (expected && !BOOST_TEST_EQ(js::detail::code_points(text), *expected)) {
            return;
        }
    }
}

void the_documented_cases() {
    BOOST_TEST_EQ(js::detail::code_points(""), 0U);
    BOOST_TEST_EQ(js::detail::code_points("abc"), 3U);
    BOOST_TEST_EQ(js::detail::code_points("\xC3\xA9"), 1U);
    BOOST_TEST_EQ(js::detail::code_points("\xF0\x9F\x90\xB2\xF0\x9F\x90\xB2"), 2U);
    BOOST_TEST_EQ(js::detail::code_points(std::string_view("a\0b", 3)), 3U);
    BOOST_TEST(js::detail::is_valid_utf8(""));
    BOOST_TEST(js::detail::is_valid_utf8(std::string_view("\0", 1)));
    BOOST_TEST(js::detail::is_valid_utf8("\xF0\x9F\x90\xB2"));
    BOOST_TEST(js::detail::is_valid_utf8("\xF4\x8F\xBF\xBF"));   // U+10FFFF
    BOOST_TEST(js::detail::is_valid_utf8("\xEF\xBF\xBF"));       // U+FFFF, a noncharacter
    BOOST_TEST(!js::detail::is_valid_utf8("\xC0\x80"));          // overlong
    BOOST_TEST(!js::detail::is_valid_utf8("\xE0\x9F\xBF"));      // overlong
    BOOST_TEST(!js::detail::is_valid_utf8("\xF0\x8F\xBF\xBF"));  // overlong
    BOOST_TEST(!js::detail::is_valid_utf8("\xED\xA0\x80"));      // a surrogate
    BOOST_TEST(!js::detail::is_valid_utf8("\xED\xBF\xBF"));      // a surrogate
    BOOST_TEST(!js::detail::is_valid_utf8("\xF4\x90\x80\x80"));  // past U+10FFFF
    BOOST_TEST(!js::detail::is_valid_utf8("\xF5\x80\x80\x80"));  // past U+10FFFF
    BOOST_TEST(!js::detail::is_valid_utf8("\xE2\x82"));          // truncated
    BOOST_TEST(!js::detail::is_valid_utf8(std::string_view("\xE2\x82\xAC", 2)));  // cut by the view
    BOOST_TEST(!js::detail::is_valid_utf8("\xF0\x9F\x90"));                       // truncated
    BOOST_TEST(!js::detail::is_valid_utf8("\x80"));
    BOOST_TEST(!js::detail::is_valid_utf8("\xFF"));
    BOOST_TEST(!js::detail::is_valid_utf8("\xC3\xA9\x80"));  // a continuation alone
}

// Every number up to 2^21 - 1, encoded: valid exactly when it is a scalar value, as one code point.
void every_number_encoded() {
    for (std::uint32_t value = 0; value < 0x200000U; ++value) {
        const std::string s = encode(value);
        if (!BOOST_TEST_EQ(js::detail::is_valid_utf8(s), is_scalar(value))) {
            return;
        }
        if (is_scalar(value) && !BOOST_TEST_EQ(js::detail::code_points(s), 1U)) {
            return;
        }
    }
}

// Every string of one or two bytes, and every string of three whose first byte is a lead byte of
// three or four.
void every_short_string() {
    for (unsigned a = 0; a < 256; ++a) {
        agrees(bytes({a}));
        for (unsigned b = 0; b < 256; ++b) {
            agrees(bytes({a, b}));
        }
    }
    for (unsigned a = 0xE0; a < 0xF8; ++a) {
        for (unsigned b = 0; b < 256; ++b) {
            for (unsigned c = 0; c < 256; ++c) {
                const std::string s = bytes({a, b, c});
                if (js::detail::is_valid_utf8(s) != oracle(s).has_value()) {
                    BOOST_ERROR("a three-byte string disagrees with the oracle");
                    return;
                }
            }
        }
    }
}

// Every lead byte of four with every second byte, and the third and fourth bytes at the edges of
// the continuation range and just past them.
void every_four_byte_lead() {
    constexpr std::array<unsigned, 9> edges{0x00, 0x7F, 0x80, 0x81, 0x8F, 0x90, 0xBF, 0xC0, 0xFF};
    for (unsigned a = 0xF0; a < 0x100; ++a) {
        for (unsigned b = 0; b < 256; ++b) {
            for (const unsigned c : edges) {
                for (const unsigned d : edges) {
                    agrees(bytes({a, b, c, d}));
                }
            }
        }
    }
}

}  // namespace

int main() {
    the_documented_cases();
    every_number_encoded();
    every_short_string();
    every_four_byte_lead();
    return boost::report_errors();
}
