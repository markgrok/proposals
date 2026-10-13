// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The instructions of a compiled plan: what each one does, and how it reads its operands.
*/
#ifndef WEBCPP_JSON_SCHEMA_DETAIL_COMPILER_OPCODE_HPP
#define WEBCPP_JSON_SCHEMA_DETAIL_COMPILER_OPCODE_HPP

#include <webcpp/json_schema/config.hpp>

#include <cstdint>
#include <string_view>

namespace webcpp::json_schema::detail {

/**
 What an instruction does, and how it reads its operands.

 An instruction's `a` and `b` name entries of the plan's tables, and a list in `operands` is
 named either by its first entry in `a` and its count in `b`, or, where the instruction names two
 lists, by the index of a table in `operands` that starts with its count. A count of the IR, a
 `std::uint64_t`, is held as its low word in `a` and its high word in `b`. A block that is absent
 is `no_block`.

 @note The values are fixed: a value is never reused for another meaning, and an instruction
 added later takes a value of its own.
*/
enum class opcode : std::uint8_t {
    /** Ends the block: its node's verdict is the block's. */
    ret = 0,
    /**
     Skips the next `a` instructions when the instance's type is not `small`, one bit of
     `ir::type_set`: `object`, `array`, `string` or `number`, which holds every integer.
    */
    type_guard = 1,
    /** Fails: the schema `false`. */
    fail = 2,
    /** `type`: `small` the types' bits; `flags` bit 0, whether `1.0` is an integer. */
    assert_type = 3,
    /** `const`: `a` the value. */
    assert_const = 4,
    /** `enum`: the values, `a` the first in `operands` and `b` their count. */
    assert_enum = 5,
    /** `multipleOf`: `a` the number. */
    assert_multiple_of = 6,
    /** `maximum`: `a` the number. */
    assert_maximum = 7,
    /** `exclusiveMaximum`: `a` the number. */
    assert_exclusive_maximum = 8,
    /** `minimum`: `a` the number. */
    assert_minimum = 9,
    /** `exclusiveMinimum`: `a` the number. */
    assert_exclusive_minimum = 10,
    /** `maxLength` or `minLength`: the count in `a` and `b`; `flags` bit 0 for the maximum. */
    assert_length = 11,
    /** `pattern`: `a` the regex. */
    assert_pattern = 12,
    /** `maxItems` or `minItems`: as @ref assert_length. */
    assert_item_count = 13,
    /** `uniqueItems: true`. */
    assert_unique = 14,
    /** `maxProperties` or `minProperties`: as @ref assert_length. */
    assert_property_count = 15,
    /** `required`: the names, `a` the first in `operands` and `b` their count. */
    assert_required = 16,
    /** `dependentRequired`: `a` and `b` the first and the count of the IR's dependencies. */
    assert_dependent_required = 17,
    /** `allOf`: the blocks, `a` the first in `operands` and `b` their count. */
    all_of = 18,
    /** `anyOf`: as @ref all_of. */
    any_of = 19,
    /** `oneOf`: as @ref all_of. */
    one_of = 20,
    /** `not`: `a` the block; the underscore keeps the name apart from the token `not`. */
    not_ = 21,  // NOLINT(readability-identifier-naming)
    /**
     `if`, with `then` and `else`: `a` the condition's block, `b` the index in `operands` of the
     `then` block, which the `else` block follows; either may be `no_block`.
    */
    if_then_else = 22,
    /**
     `properties`, `patternProperties` and `additionalProperties`, one loop over the members: `a`
     the index in `operands` of the name table, its count then its (name, block) pairs sorted by
     the names' bytes; `b` the index of the additional block, then the pattern table's count and
     its (regex, block) pairs in source order.
    */
    loop_properties = 23,
    /** `propertyNames`: `a` the block. */
    loop_property_names = 24,
    /** `dependentSchemas`: the (name, block) pairs, `a` the first in `operands`, `b` their count. */
    loop_dependent_schemas = 25,
    /**
     `prefixItems` and `items`, one loop over the items: `a` the index in `operands` of the
     prefix's count, then its blocks; `b` the block of the items after it.
    */
    loop_items = 26,
    /**
     `contains`, with `minContains` and `maxContains`: `a` the block; `b` the index in `operands`
     of the minimum's two words, then the maximum's; `flags` bit 0, whether there is a maximum.
    */
    loop_contains = 27,
};

/**
 The name of an opcode, as the plan's text writes it: the enumerator's, and `not` for
 @ref opcode::not_.

 @param op The opcode.
 @return Its name, or `"unknown"` for a value that names no opcode.
*/
constexpr std::string_view opcode_name(opcode op) noexcept {
    switch (op) {
        case opcode::ret: return "ret";
        case opcode::type_guard: return "type_guard";
        case opcode::fail: return "fail";
        case opcode::assert_type: return "assert_type";
        case opcode::assert_const: return "assert_const";
        case opcode::assert_enum: return "assert_enum";
        case opcode::assert_multiple_of: return "assert_multiple_of";
        case opcode::assert_maximum: return "assert_maximum";
        case opcode::assert_exclusive_maximum: return "assert_exclusive_maximum";
        case opcode::assert_minimum: return "assert_minimum";
        case opcode::assert_exclusive_minimum: return "assert_exclusive_minimum";
        case opcode::assert_length: return "assert_length";
        case opcode::assert_pattern: return "assert_pattern";
        case opcode::assert_item_count: return "assert_item_count";
        case opcode::assert_unique: return "assert_unique";
        case opcode::assert_property_count: return "assert_property_count";
        case opcode::assert_required: return "assert_required";
        case opcode::assert_dependent_required: return "assert_dependent_required";
        case opcode::all_of: return "all_of";
        case opcode::any_of: return "any_of";
        case opcode::one_of: return "one_of";
        case opcode::not_: return "not";
        case opcode::if_then_else: return "if_then_else";
        case opcode::loop_properties: return "loop_properties";
        case opcode::loop_property_names: return "loop_property_names";
        case opcode::loop_dependent_schemas: return "loop_dependent_schemas";
        case opcode::loop_items: return "loop_items";
        case opcode::loop_contains: return "loop_contains";
    }
    return "unknown";
}

}  // namespace webcpp::json_schema::detail

#endif  // WEBCPP_JSON_SCHEMA_DETAIL_COMPILER_OPCODE_HPP
