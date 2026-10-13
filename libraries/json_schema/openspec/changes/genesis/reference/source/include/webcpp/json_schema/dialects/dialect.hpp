// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The dialects json_schema reads: their identifiers, their tags, and the `$schema` URI of each.

 @see "Dialects", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_DIALECTS_DIALECT_HPP
#define WEBCPP_JSON_SCHEMA_DIALECTS_DIALECT_HPP

#include <webcpp/json_schema/config.hpp>

#include <concepts>
#include <cstdint>
#include <optional>
#include <string_view>

namespace webcpp::json_schema {

/**
 A dialect of JSON Schema.

 @note The values are fixed: they are never reused for a different meaning, so a stored value
 keeps its meaning across versions, and a dialect added later takes the next one.
 @see "Dialects", in the guide.
*/
enum class dialect : std::uint8_t {
    /** Draft 4. */
    draft4 = 1,
    /** Draft 6. */
    draft6 = 2,
    /** Draft 7. */
    draft7 = 3,
    /** Draft 2019-09. */
    draft2019_09 = 4,
    /** Draft 2020-12. */
    draft2020_12 = 5,
    /** The Schema Object of OpenAPI 3.0, which no `$schema` names. */
    openapi3_0 = 6,
};

/** The tag of draft 4. */
struct draft4 {
    /** Its dialect. */
    static constexpr dialect id = dialect::draft4;
};

/** The tag of draft 6. */
struct draft6 {
    /** Its dialect. */
    static constexpr dialect id = dialect::draft6;
};

/** The tag of draft 7. */
struct draft7 {
    /** Its dialect. */
    static constexpr dialect id = dialect::draft7;
};

/** The tag of draft 2019-09. */
struct draft2019_09 {
    /** Its dialect. */
    static constexpr dialect id = dialect::draft2019_09;
};

/** The tag of draft 2020-12. */
struct draft2020_12 {
    /** Its dialect. */
    static constexpr dialect id = dialect::draft2020_12;
};

/** The tag of OpenAPI 3.0's Schema Object. */
struct openapi3_0 {
    /** Its dialect. */
    static constexpr dialect id = dialect::openapi3_0;
};

namespace detail {

/**
 The machinery of a dialect: its keyword table, specialized in `detail/dialects/` for each tag
 that @ref has_machinery names.

 @tparam D The dialect's tag.
*/
template <class D>
struct dialect_machinery;

/**
 Whether json_schema loads the dialect of the tag `D`: its machinery exists.

 Tip: it is stated here, beside the tags, and not read from the machinery, so that whether a tag
 is a @ref dialect_policy is the same wherever the tag is visible, whatever was included first.
 The validator checks that each of its dialects' machinery is defined.
*/
template <class D>
inline constexpr bool has_machinery = false;

/** Draft 2020-12 is loaded. */
template <>
inline constexpr bool has_machinery<draft2020_12> = true;

}  // namespace detail

/**
 A dialect json_schema loads: a tag whose `id` names its @ref dialect, and whose dialect has its
 machinery, the table of the keywords it reads.

 @tparam D The tag.
 @see "Dialects", in the guide.
*/
template <class D>
concept dialect_policy = requires {
    { D::id } -> std::convertible_to<dialect>;
} && detail::has_machinery<D>;

/**
 The `$schema` URI of a dialect, spelled as its meta-schema spells its own: with the empty fragment
 up to draft 7, without it from 2019-09.

 @param d The dialect.
 @return The URI, or `""` for OpenAPI 3.0, which no `$schema` names.
 @see "Dialects", in the guide.
*/
constexpr std::string_view uri_of(dialect d) noexcept {
    switch (d) {
        case dialect::draft4: return "http://json-schema.org/draft-04/schema#";
        case dialect::draft6: return "http://json-schema.org/draft-06/schema#";
        case dialect::draft7: return "http://json-schema.org/draft-07/schema#";
        case dialect::draft2019_09: return "https://json-schema.org/draft/2019-09/schema";
        case dialect::draft2020_12: return "https://json-schema.org/draft/2020-12/schema";
        case dialect::openapi3_0: return "";
    }
    return "";
}

/**
 The dialect a `$schema` URI names: an official meta-schema's URI, with or without its empty
 fragment, byte for byte.

 @param uri The URI.
 @return The dialect, or nothing for any other URI.
 @see "Dialects", in the guide.
*/
constexpr std::optional<dialect> dialect_of_uri(std::string_view uri) noexcept {
    if (uri.ends_with('#')) {
        uri.remove_suffix(1);
    }
    for (const dialect d : {
             dialect::draft4,
             dialect::draft6,
             dialect::draft7,
             dialect::draft2019_09,
             dialect::draft2020_12,
         }) {
        std::string_view official = uri_of(d);
        if (official.ends_with('#')) {
            official.remove_suffix(1);
        }
        if (uri == official) {
            return d;
        }
    }
    return std::nullopt;
}

}  // namespace webcpp::json_schema

#endif  // WEBCPP_JSON_SCHEMA_DIALECTS_DIALECT_HPP
