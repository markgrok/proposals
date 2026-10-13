// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The rows of a dialect's keyword table: each keyword's name, the kind it loads as, how its value is
 read, and the vocabulary it belongs to.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_DIALECTS_KEYWORD_TABLE_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_DIALECTS_KEYWORD_TABLE_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/ir/keyword.hpp>

#include <cstdint>
#include <span>
#include <string_view>

namespace webcpp::json_schema::detail {

/**
 How a keyword's value is read into its payload: the shape it accepts, and what it builds.

 @note The values are appended, never reordered: a dialect's table names them.
*/
enum class reader : std::uint8_t {
    /** One subschema: any value, refused by the walk where it is no schema. */
    schema,
    /** An array of subschemas. */
    schema_list,
    /** A non-empty array of subschemas. */
    nonempty_schema_list,
    /** An object of subschemas, in source order. */
    schema_map,
    /** An object of subschemas whose names are patterns. */
    pattern_map,
    /** A type name, or a non-empty array of distinct ones. */
    types,
    /** Any value, `uniqueItems` a boolean: a view of it. */
    constant,
    /** An array: a view of each element. */
    constants,
    /** A number. */
    bound,
    /** A number above zero. */
    positive_bound,
    /** A non-negative integer, `2.0` included. */
    count,
    /** A string. */
    text,
    /** A string, a regular expression compiled by the compiler. */
    pattern,
    /** An array of distinct strings. */
    names,
    /** An object of arrays of distinct strings. */
    dependent_required,
    /** A string, the format's name. */
    format,
    /** One subschema, whose matches annotate. */
    contains,
    /** A string, the URI of a reference. */
    ref,
    /** A string, the URI of a dynamic reference. */
    dynamic_ref,
    /** A value of the shape its annotation's meta-schema gives: a view of it. */
    annotation,
};

/** One row of a dialect's keyword table. */
struct keyword_rule {
    /** The keyword's name, as a schema spells it. */
    std::string_view name{};

    /** The kind it loads as. */
    ir::keyword_kind kind = ir::keyword_kind::unknown;

    /** How its value is read. */
    reader read = reader::constant;

    /** The vocabulary it belongs to, one of `ir::vocabulary_bit`. */
    std::uint16_t vocabulary = 0;
};

/**
 The row of `name` in `table`, or nullptr when the table has none.

 Tip: a table holds a few dozen rows, and a comparison of two names stops at their lengths first,
 so a scan costs less than an index would to build.
*/
constexpr const keyword_rule* rule_named(std::span<const keyword_rule> table,
                                         std::string_view name) noexcept {
    for (const keyword_rule& rule : table) {
        if (rule.name == name) {
            return &rule;
        }
    }
    return nullptr;
}

/** The vocabularies of a table's keywords, an OR of their `ir::vocabulary_bit` values. */
constexpr std::uint16_t vocabularies_of(std::span<const keyword_rule> table) noexcept {
    std::uint16_t bits = 0;
    for (const keyword_rule& rule : table) {
        bits = static_cast<std::uint16_t>(bits | rule.vocabulary);
    }
    return bits;
}

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_DIALECTS_KEYWORD_TABLE_HPP
