// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The keywords of the IR: their kinds, with fixed values, and the payload each kind holds.

 @see "The IR", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_IR_KEYWORD_HPP
#define WEBCPP_JSON_SCHEMA_IR_KEYWORD_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/ir/ids.hpp>

#include <cstddef>
#include <cstdint>
#include <variant>

namespace webcpp::json_schema::ir {

/**
 A set of JSON types, in which integer is a type of its own (validation 6.1.1).

 @see "The IR", in the guide.
*/
struct type_set {
    /** One type of the set. */
    enum bit : std::uint8_t {
        /** `null`. */
        null = 1,
        /** `boolean`. */
        boolean = 2,
        /** `object`. */
        object = 4,
        /** `array`. */
        array = 8,
        /** `number`, which holds every integer too. */
        number = 16,
        /** `string`. */
        string = 32,
        /** `integer`. */
        integer = 64,
    };

    /**
     The set of every type: the seven bits, `null` to `integer`.

     @return The set of the seven types.
    */
    static constexpr type_set all() noexcept { return {.bits = 0x7F}; }

    /**
     Whether the set holds a type.

     @param b The type.
     @return `true` when `b` is in the set.
    */
    [[nodiscard]] constexpr bool contains(bit b) const noexcept { return (bits & b) != 0; }

    /** The types of the set, an OR of @ref bit values. */
    std::uint8_t bits = 0;
};

/**
 The kind of a keyword, after normalization: what it means, whatever its dialect spells it.

 @note The values are fixed: a value is never reused for another meaning, and a kind added later
 takes a value of its own.
 @see "The IR", in the guide.
*/
enum class keyword_kind : std::uint8_t {
    /** `$ref`, resolved. */
    ref = 1,
    /** `$dynamicRef` or `$recursiveRef`. */
    dynamic_ref = 2,
    /** `$defs` or `definitions`. */
    defs = 3,
    /** `$comment`. */
    comment = 4,
    /** `allOf`. */
    all_of = 10,
    /** `anyOf`. */
    any_of = 11,
    /** `oneOf`. */
    one_of = 12,
    /** `not`; the underscore keeps the name apart from the alternative token `not`. */
    not_ = 13,  // NOLINT(readability-identifier-naming)
    /** `if`; the underscore keeps the name apart from the keyword `if`. */
    if_ = 14,  // NOLINT(readability-identifier-naming)
    /** `then`, kept without `if`, where it means nothing; its underscore matches `if_`'s. */
    then_ = 15,  // NOLINT(readability-identifier-naming)
    /** `else`, kept without `if`, where it means nothing; the underscore keeps it from `else`. */
    else_ = 16,  // NOLINT(readability-identifier-naming)
    /** `dependentSchemas`, or the schemas of `dependencies`. */
    dependent_schemas = 17,
    /** `prefixItems`, or the array form of `items` before 2020-12. */
    prefix_items = 18,
    /** `items` of 2020-12, the schema form of `items` before it, or `additionalItems`. */
    items = 19,
    /** `contains`. */
    contains = 20,
    /** `properties`. */
    properties = 21,
    /** `patternProperties`, whose names are the patterns. */
    pattern_properties = 22,
    /** `additionalProperties`. */
    additional_properties = 23,
    /** `propertyNames`. */
    property_names = 24,
    /** `unevaluatedItems`. */
    unevaluated_items = 30,
    /** `unevaluatedProperties`. */
    unevaluated_properties = 31,
    /** `type`. */
    type = 40,
    /** `enum`; the underscore keeps the name apart from the keyword `enum`. */
    enum_ = 41,  // NOLINT(readability-identifier-naming)
    /** `const`; the underscore keeps the name apart from the keyword `const`. */
    const_ = 42,  // NOLINT(readability-identifier-naming)
    /** `multipleOf`, greater than zero. */
    multiple_of = 43,
    /** `maximum`. */
    maximum = 44,
    /** `exclusiveMaximum`, draft 4's boolean form normalized. */
    exclusive_maximum = 45,
    /** `minimum`. */
    minimum = 46,
    /** `exclusiveMinimum`, draft 4's boolean form normalized. */
    exclusive_minimum = 47,
    /** `maxLength`. */
    max_length = 48,
    /** `minLength`. */
    min_length = 49,
    /** `pattern`. */
    pattern = 50,
    /** `maxItems`. */
    max_items = 51,
    /** `minItems`. */
    min_items = 52,
    /** `uniqueItems`, the boolean as written. */
    unique_items = 53,
    /** `maxContains`. */
    max_contains = 54,
    /** `minContains`. */
    min_contains = 55,
    /** `maxProperties`. */
    max_properties = 56,
    /** `minProperties`. */
    min_properties = 57,
    /** `required`. */
    required = 58,
    /** `dependentRequired`, or the arrays of `dependencies`. */
    dependent_required = 59,
    /** `format`. */
    format = 60,
    /** `contentEncoding`. */
    content_encoding = 61,
    /** `contentMediaType`. */
    content_media_type = 62,
    /** `contentSchema`. */
    content_schema = 63,
    /** `title`. */
    title = 70,
    /** `description`. */
    description = 71,
    /** `default`; the underscore keeps the name apart from the keyword `default`. */
    default_ = 72,  // NOLINT(readability-identifier-naming)
    /** `deprecated`. */
    deprecated = 73,
    /** `readOnly`. */
    read_only = 74,
    /** `writeOnly`. */
    write_only = 75,
    /** `examples`. */
    examples = 76,
    /** `discriminator`, an annotation of OpenAPI 3.0. */
    discriminator = 80,
    /** `example`, an annotation of OpenAPI 3.0. */
    example = 81,
    /** `xml`, an annotation of OpenAPI 3.0. */
    xml = 82,
    /** `externalDocs`, an annotation of OpenAPI 3.0. */
    external_docs = 83,
    /** A keyword no vocabulary in use defines: an annotation or nothing, as the options say. */
    unknown = 90,
    /** A keyword its dialect ignores, kept for the emitter and never compiled. */
    ignored = 91,
    /** A keyword of a user vocabulary. */
    user = 92,
};

/** How a dynamic reference is written. */
enum class dynamic_form : std::uint8_t {
    /** `$dynamicRef`, of 2020-12. */
    dynamic = 0,
    /** `$recursiveRef`, of 2019-09. */
    recursive = 1,
};

/** One subschema: `not`, `if`, `items` and the other keywords of one schema. */
struct subschema {
    /** The subschema. */
    node_id schema{};
};

/** A list of subschemas: `allOf`, `anyOf`, `oneOf`, `prefixItems`. */
struct subschema_list {
    /** The subschemas, a slice of the document's node lists, in source order. */
    range nodes{};
};

/** A subschema under a name: an entry of `properties`, `patternProperties` or `$defs`. */
struct named_subschema {
    /** The name: a property's, a pattern, or a definition's. */
    string_id name{};

    /** The subschema. */
    node_id schema{};
};

/** Named subschemas: `properties`, `patternProperties`, `$defs`, `dependentSchemas`. */
struct named_subschemas {
    /** The entries, a slice of the document's entries, in source order. */
    range entries{};
};

/** A reference resolved when the schema was loaded: `$ref`. */
struct reference {
    /** The node it refers to. */
    node_id target{};

    /** The URI as written, for the emitter, `explain` and the output. */
    string_id uri{};
};

/** A dynamic reference: `$dynamicRef` or `$recursiveRef`. */
struct dynamic_reference {
    /** The node it refers to when no dynamic scope applies: the static resolution. */
    node_id initial_target{};

    /** The URI as written. */
    string_id uri{};

    /** The name of the fragment, the anchor looked for; `""` for `$recursiveRef`. */
    string_id anchor{};

    /** How it is written. */
    dynamic_form form = dynamic_form::dynamic;

    /**
     Whether the initial target carries the matching dynamic anchor (2020-12), or is a resource
     root with `$recursiveAnchor` (2019-09); when it does not, the reference is a plain one.
    */
    bool bookended = false;
};

/** The types `type` allows. */
struct types {
    /** The types. */
    type_set set{};

    /** Whether a number without a fraction is an integer; false in draft 4 only. */
    bool integer_by_value = true;
};

/** One JSON value: `const`, `$comment`, the annotations, a keyword kept as written. */
struct constant {
    /** The value. */
    value_id value{};
};

/** A list of JSON values: `enum`. */
struct constants {
    /** The values, a slice of the document's value lists, in source order. */
    range values{};
};

/** A number operand: `multipleOf` and the four bounds. */
struct bound {
    /** The number, exact. */
    number_id value{};
};

/** A count: `maxLength`, `minItems` and the other limits on a size. */
struct count {
    /** The count. */
    std::uint64_t value = 0;
};

/** A text: `pattern`, `contentEncoding`, `contentMediaType`. */
struct text {
    /** The text. */
    string_id value{};
};

/** `format`, with whether it asserts, which its dialect and vocabularies decide. */
struct format_payload {
    /** The format's name. */
    string_id name{};

    /** Whether an instance that is not of the format fails; else the format only annotates. */
    bool assertion = false;
};

/** A list of names: `required`. */
struct names {
    /** The names, a slice of the document's string lists, as written. */
    range strings{};
};

/** The names one property requires: an entry of `dependentRequired`. */
struct dependency {
    /** The property. */
    string_id name{};

    /** The names it requires, a slice of the document's string lists. */
    range strings{};
};

/** `dependentRequired`. */
struct dependencies {
    /** The entries, a slice of the document's dependencies, in source order. */
    range entries{};
};

/** `contains`, with whether it annotates the items it matched. */
struct contains_payload {
    /** The subschema. */
    node_id schema{};

    /** Whether it annotates the indices it matched, which `unevaluatedItems` reads; from 2020-12.
     */
    bool annotates_items = false;
};

/** A keyword of a user vocabulary, by the vocabulary's index and the keyword's index in it. */
struct user_payload {
    /** The vocabulary. */
    std::uint16_t vocabulary = 0;

    /** The keyword, within its vocabulary. */
    std::uint16_t keyword = 0;
};

/**
 What a keyword holds: the alternative @ref payload_index gives for its kind.

 The alternatives' indices are part of the contract: `std::monostate` is 0, which no kind holds,
 and `user_payload` is 16.
*/
using payload = std::variant<std::monostate, subschema, subschema_list, named_subschemas, reference,
                             dynamic_reference, types, constant, constants, bound, count, text,
                             format_payload, names, dependencies, contains_payload, user_payload>;

/**
 One keyword of a node.

 @see "The IR", in the guide.
*/
struct keyword {
    /** What it means; @ref keyword_kind::unknown until it is set. */
    keyword_kind kind = keyword_kind::unknown;

    /**
     The name as written in the source (`items`, `additionalItems`, `definitions`), or the
     canonical name when built in code.
    */
    string_id name{};

    /** The keyword's value as written, or @ref no_value when built in code. */
    value_id source = no_value;

    /** What it holds, the alternative @ref payload_index gives for `kind`. */
    payload data;
};

/**
 The index, in @ref payload, of the alternative a keyword of a kind holds.

 @param kind The kind.
 @return The index; 0, which no kind holds, for a value that names no kind.
*/
constexpr std::size_t payload_index(keyword_kind kind) noexcept {
    using k = keyword_kind;
    switch (kind) {
        case k::not_:
        case k::if_:
        case k::then_:
        case k::else_:
        case k::items:
        case k::additional_properties:
        case k::property_names:
        case k::unevaluated_items:
        case k::unevaluated_properties:
        case k::content_schema: return 1;
        case k::all_of:
        case k::any_of:
        case k::one_of:
        case k::prefix_items: return 2;
        case k::defs:
        case k::dependent_schemas:
        case k::properties:
        case k::pattern_properties: return 3;
        case k::ref: return 4;
        case k::dynamic_ref: return 5;
        case k::type: return 6;
        case k::comment:
        case k::const_:
        case k::unique_items:
        case k::title:
        case k::description:
        case k::default_:
        case k::deprecated:
        case k::read_only:
        case k::write_only:
        case k::examples:
        case k::discriminator:
        case k::example:
        case k::xml:
        case k::external_docs:
        case k::unknown:
        case k::ignored: return 7;
        case k::enum_: return 8;
        case k::multiple_of:
        case k::maximum:
        case k::exclusive_maximum:
        case k::minimum:
        case k::exclusive_minimum: return 9;
        case k::max_length:
        case k::min_length:
        case k::max_items:
        case k::min_items:
        case k::max_contains:
        case k::min_contains:
        case k::max_properties:
        case k::min_properties: return 10;
        case k::pattern:
        case k::content_encoding:
        case k::content_media_type: return 11;
        case k::format: return 12;
        case k::required: return 13;
        case k::dependent_required: return 14;
        case k::contains: return 15;
        case k::user: return 16;
    }
    return 0;
}

}  // namespace webcpp::json_schema::ir

#endif  // WEBCPP_JSON_SCHEMA_IR_KEYWORD_HPP
