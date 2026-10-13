// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The indices the IR names its parts by: a node, a keyword, a resource, a string, a value, a number
 and a JSON Pointer of one document, a slice of one of its side tables, and a step of a pointer.

 Tip: the IR holds indices only, never an address, so that a document copied with another
 allocator names the same parts by the same values.

 @see "The IR", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_IR_IDS_HPP
#define WEBCPP_JSON_SCHEMA_IR_IDS_HPP

#include <webcpp/json_schema/config.hpp>

#include <cstdint>

namespace webcpp::json_schema::ir {

/** The index of a node, a schema, in its document. */
enum class node_id : std::uint32_t {};

/** The index of a keyword in its document's keyword table. */
enum class keyword_id : std::uint32_t {};

/** The index of a schema resource in its document. */
enum class resource_id : std::uint32_t {};

/** The index of an interned string in its document. */
enum class string_id : std::uint32_t {};

/** The index of a JSON value in its document. */
enum class value_id : std::uint32_t {};

/** The index of a number operand in its document. */
enum class number_id : std::uint32_t {};

/** The index of a JSON Pointer in its document: the index of its last step. */
enum class pointer_id : std::uint32_t {};

/** No node: the target of a reference not yet resolved, which a checked document never holds. */
inline constexpr node_id no_node{0xFFFF'FFFFU};

/** No value: the source of a keyword built in code, which has no JSON text. */
inline constexpr value_id no_value{0xFFFF'FFFFU};

/** The empty JSON Pointer, `""`: the whole value, where every chain of steps starts. */
inline constexpr pointer_id empty_pointer{0xFFFF'FFFFU};

/**
 A slice of one of the document's side tables: `count` entries from `first`.

 @see "The IR", in the guide.
*/
struct range {
    /** The index of the slice's first entry. */
    std::uint32_t first = 0;

    /** How many entries the slice holds. */
    std::uint32_t count = 0;
};

/**
 One step of a JSON Pointer: the pointer it extends, then a member's name or an element's index.

 A pointer is held as its last step, which names its parent's, so the pointers of a document hold
 each token once, however long the pointers that share it.

 @see "The IR", in the guide.
*/
struct pointer_step {
    /** The pointer it extends: @ref empty_pointer, or a step that comes before it. */
    pointer_id parent = empty_pointer;

    /** The member's name, as written, not escaped; unread for an element. */
    string_id name{};

    /** The element's index; unread for a member. */
    std::uint32_t index = 0;

    /** Whether it steps into an array's element; else into an object's member. */
    bool element = false;
};

}  // namespace webcpp::json_schema::ir

#endif  // WEBCPP_JSON_SCHEMA_IR_IDS_HPP
