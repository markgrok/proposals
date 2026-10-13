// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 JSON Pointers (RFC 6901): written a token at a time, checked, and resolved against a value
 without recursion, in time linear in the pointer.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_JSON_POINTER_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_JSON_POINTER_HPP

#include <webcpp/json_schema/config.hpp>

#include <boost/json/value.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace webcpp::json_schema::detail {

/**
 Appends to `out` the token of the member `name`: a slash, then `name` with each `~` written
 `~0` and each `/` written `~1`. Every other byte is kept, a NUL byte included.
*/
inline void append_token(std::pmr::string& out, std::string_view name) {
    out.push_back('/');
    for (const char c : name) {
        if (c == '~') {
            out.append("~0");
        } else if (c == '/') {
            out.append("~1");
        } else {
            out.push_back(c);
        }
    }
}

/** Appends to `out` the token of the element `index`: a slash, then its decimal digits. */
inline void append_index(std::pmr::string& out, std::uint64_t index) {
    std::array<char, 24> digits{};
    const auto written = std::to_chars(digits.data(), digits.data() + digits.size(), index);
    out.push_back('/');
    out.append(digits.data(), written.ptr);
}

/**
 Appends to `out` the JSON Pointer `pointer` as a URI fragment (RFC 6901 section 6): each byte
 outside RFC 3986's fragment characters (section 3.5), and `%`, as `%XX` of its UTF-8.

 The one writer of an absolute keyword location's fragment: the collecting sink writes a unit's
 through it, and the machine writes an error's through it, so that the two agree. Decoding the
 fragment gives `pointer` back, and `~`, `/` and `$` are kept, so `/$defs/a~1b` is unchanged.

 Tip: the ASCII tests are explicit, never `std::isalnum`, which reads the locale.
*/
inline void append_fragment(std::pmr::string& out, std::string_view pointer) {
    constexpr std::string_view hex = "0123456789ABCDEF";
    constexpr std::string_view kept = "-._~!$&'()*+,;=:@/?";
    for (const char c : pointer) {
        const auto byte = static_cast<unsigned char>(c);
        const bool alnum = (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
                           (byte >= '0' && byte <= '9');
        if (alnum || kept.find(c) != std::string_view::npos) {
            out.push_back(c);
        } else {
            out.push_back('%');
            out.push_back(hex[byte >> 4U]);
            out.push_back(hex[byte & 0xFU]);
        }
    }
}

/**
 Whether `pointer` is a JSON Pointer: empty, or tokens each led by a slash, in which every `~` is
 followed by `0` or `1`.
*/
inline bool is_valid_pointer(std::string_view pointer) noexcept {
    if (!pointer.empty() && pointer.front() != '/') {
        return false;
    }
    for (std::size_t i = 0; i < pointer.size(); ++i) {
        const bool escape = pointer[i] == '~';
        if (escape &&
            (i + 1 == pointer.size() || (pointer[i + 1] != '0' && pointer[i + 1] != '1'))) {
            return false;
        }
    }
    return true;
}

/** The array index a token spells (RFC 6901 section 4): digits, no leading zero, never "-". */
inline std::optional<std::size_t> index_of(std::string_view token) noexcept {
    if (token.empty() || (token.size() > 1 && token.front() == '0')) {
        return std::nullopt;
    }
    std::size_t value = 0;
    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size()) {
        return std::nullopt;
    }
    return value;
}

/**
 Whether the escaped `token` spells `key`, reading `~0` as `~` and `~1` as `/`, copying nothing.

 A token that is not well formed, a `~` last or before another byte than `0` or `1`, spells no
 key.
*/
inline bool token_equals(std::string_view token, std::string_view key) noexcept {
    std::size_t k = 0;
    for (std::size_t t = 0; t < token.size(); ++t, ++k) {
        char c = token[t];
        if (c == '~') {
            if (t + 1 == token.size() || (token[t + 1] != '0' && token[t + 1] != '1')) {
                return false;
            }
            c = token[++t] == '0' ? '~' : '/';
        }
        if (k == key.size() || key[k] != c) {
            return false;
        }
    }
    return k == key.size();
}

/**
 One step: the member of `object` whose key the escaped `token` spells, or nullptr.

 A token without `~` is its key; one with `~` is unescaped into `scratch` first. Either way the key
 is looked up by Boost.JSON's index of the object, never compared with each key in turn, so a step
 costs its token's length, however many keys the object holds and however many of them hold `~`.

 @param scratch A string that holds the unescaped key: `clear`, `push_back`, `data` and `size`.
*/
template <class String>
const boost::json::value* member_of(const boost::json::object& object, std::string_view token,
                                    String& scratch) {
    if (token.find('~') == std::string_view::npos) {
        return object.if_contains(token);
    }
    scratch.clear();
    for (std::size_t t = 0; t < token.size(); ++t) {
        char c = token[t];
        if (c == '~') {
            c = token[++t] == '0' ? '~' : '/';
        }
        scratch.push_back(c);
    }
    return object.if_contains(std::string_view(scratch.data(), scratch.size()));
}

/**
 The value `pointer` names in `root`, or nullptr when it is not a JSON Pointer or names nothing:
 a member that is absent, an index that is not one or is past the end, `-`, or a step into a
 string, a number, a boolean or null.

 It costs the pointer's length: each token is read once, and a token holding `~` is unescaped once
 into `scratch` (@ref member_of).

 @param scratch A string that holds an unescaped key while it is looked up.
 @throws std::bad_alloc When `scratch` cannot grow; without exceptions, the program ends.
*/
template <class String>
const boost::json::value* resolve_pointer(const boost::json::value& root, std::string_view pointer,
                                          String& scratch) {
    if (!is_valid_pointer(pointer)) {
        return nullptr;
    }
    const boost::json::value* at = &root;
    while (!pointer.empty() && at != nullptr) {
        pointer.remove_prefix(1);
        const std::size_t end = std::min(pointer.find('/'), pointer.size());
        std::string_view token = pointer;
        token.remove_suffix(pointer.size() - end);
        pointer.remove_prefix(end);
        if (const auto* object = at->if_object()) {
            at = member_of(*object, token, scratch);
        } else if (const auto* array = at->if_array()) {
            const auto index = index_of(token);
            at = index && *index < array->size() ? &(*array)[*index] : nullptr;
        } else {
            at = nullptr;
        }
    }
    return at;
}

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_JSON_POINTER_HPP
