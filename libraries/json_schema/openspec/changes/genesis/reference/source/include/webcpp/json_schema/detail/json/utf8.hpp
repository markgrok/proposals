// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 UTF-8 as JSON Schema reads a string: whether bytes are UTF-8, and how many code points they hold,
 which is a string's length for `minLength` and `maxLength`.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_JSON_UTF8_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_JSON_UTF8_HPP

#include <webcpp/json_schema/config.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>

namespace webcpp::json_schema::detail {

/** The count of continuation bytes after a lead byte, and the bounds of the first one. */
struct utf8_lead {
    /** How many continuation bytes follow: 1, 2 or 3. */
    std::size_t extra;

    /** The least first continuation byte, past which no form is overlong or a surrogate. */
    unsigned char low;

    /** The greatest first continuation byte. */
    unsigned char high;
};

/**
 What the lead byte `c` of a sequence of two to four bytes asks of the bytes after it, after RFC
 3629's table (section 4); nothing for a byte that leads no sequence: an ASCII byte, a continuation
 byte, `0xC0`, `0xC1` and `0xF5` to `0xFF`.
*/
inline std::optional<utf8_lead> lead_of(unsigned char c) noexcept {
    if (c >= 0xC2 && c <= 0xDF) {
        return utf8_lead{.extra = 1, .low = 0x80, .high = 0xBF};
    }
    if (c >= 0xE0 && c <= 0xEF) {
        return utf8_lead{
            .extra = 2,
            .low = static_cast<unsigned char>(c == 0xE0 ? 0xA0 : 0x80),   // overlong below
            .high = static_cast<unsigned char>(c == 0xED ? 0x9F : 0xBF),  // surrogates above
        };
    }
    if (c >= 0xF0 && c <= 0xF4) {
        return utf8_lead{
            .extra = 3,
            .low = static_cast<unsigned char>(c == 0xF0 ? 0x90 : 0x80),   // overlong below
            .high = static_cast<unsigned char>(c == 0xF4 ? 0x8F : 0xBF),  // past U+10FFFF above
        };
    }
    return std::nullopt;
}

/**
 Whether `s` is UTF-8 (RFC 3629): every sequence whole, in its shortest form, and a scalar value,
 neither a surrogate nor past U+10FFFF.
*/
inline bool is_valid_utf8(std::string_view s) noexcept {
    std::size_t i = 0;
    while (i < s.size()) {
        const auto c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            ++i;
            continue;
        }
        const auto lead = lead_of(c);
        if (!lead || s.size() - i <= lead->extra) {
            return false;
        }
        for (std::size_t k = 1; k <= lead->extra; ++k) {
            const auto t = static_cast<unsigned char>(s[i + k]);
            if (t < (k == 1 ? lead->low : 0x80) || t > (k == 1 ? lead->high : 0xBF)) {
                return false;
            }
        }
        i += lead->extra + 1;
    }
    return true;
}

/**
 The count of code points in `s`, which must be UTF-8: its bytes outside `0x80` to `0xBF`, each of
 which starts one.
*/
inline std::size_t code_points(std::string_view s) noexcept {
    return static_cast<std::size_t>(std::count_if(s.begin(), s.end(), [](char c) {
        const auto b = static_cast<unsigned char>(c);
        return b < 0x80 || b > 0xBF;
    }));
}

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_JSON_UTF8_HPP
