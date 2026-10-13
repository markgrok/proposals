// Copyright (c) 2026 WebCpp.org
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt)

/**
 The nodes of the IR, the schema resources they belong to, and the anchors that name them.

 @see "The IR", in the guide.
*/
#ifndef WEBCPP_JSON_SCHEMA_IR_NODE_HPP
#define WEBCPP_JSON_SCHEMA_IR_NODE_HPP

#include <webcpp/json_schema/config.hpp>

#include <webcpp/json_schema/dialects/dialect.hpp>
#include <webcpp/json_schema/ir/ids.hpp>

#include <cstdint>

namespace webcpp::json_schema::ir {

/** What a node is: a boolean schema, or an object schema with its keywords. */
enum class node_kind : std::uint8_t {
    /** The schema `true`, which every instance satisfies. */
    always_valid = 1,
    /** The schema `false`, which no instance satisfies. */
    never_valid = 2,
    /** An object schema. */
    object = 3,
};

/**
 A schema: its kind, its resource, its place in the resource, and its keywords.

 @see "The IR", in the guide.
*/
struct node {
    /** What it is. */
    node_kind kind = node_kind::never_valid;

    /** The schema resource it belongs to. */
    resource_id resource{};

    /** Its JSON Pointer from its resource's root: @ref empty_pointer for the root. */
    pointer_id pointer = empty_pointer;

    /** Its keywords, contiguous in the document's keyword table, in source order. */
    range keywords{};
};

/**
 The bits of @ref resource::vocabularies, one per vocabulary of 2019-09 and 2020-12, which a
 resource records and nothing reads for its meaning.
*/
namespace vocabulary_bit {

/** The core vocabulary. */
inline constexpr std::uint16_t core = 1;

/** The applicator vocabulary. */
inline constexpr std::uint16_t applicator = 2;

/** The unevaluated vocabulary. */
inline constexpr std::uint16_t unevaluated = 4;

/** The validation vocabulary. */
inline constexpr std::uint16_t validation = 8;

/** The meta-data vocabulary. */
inline constexpr std::uint16_t meta_data = 16;

/** The format-annotation vocabulary. */
inline constexpr std::uint16_t format_annotation = 32;

/** The format-assertion vocabulary. */
inline constexpr std::uint16_t format_assertion = 64;

/** The content vocabulary. */
inline constexpr std::uint16_t content = 128;

}  // namespace vocabulary_bit

/**
 A schema resource: a schema with a base URI of its own, and the nodes within it.

 @see "The IR", in the guide.
*/
struct resource {
    /** Its base URI: absolute, normalized, without a fragment; `urn:webcpp:root` when none. */
    string_id uri{};

    /**
     The dialect it was written in, for the emitter, `$schema` in the output and `explain`; never
     for what a keyword means.
    */
    dialect source_dialect = dialect::draft2020_12;

    /** Its root node, whose pointer is @ref empty_pointer. */
    node_id root{};

    /** The resource that lexically encloses it; itself for the document's root resource. */
    resource_id parent{};

    /** The vocabularies it was loaded with, an OR of @ref vocabulary_bit values; a record only. */
    std::uint16_t vocabularies = 0;

    /** Whether its root has 2019-09's `"$recursiveAnchor": true`. */
    bool recursive_anchor = false;
};

/**
 A name for a node within its resource: `$anchor`, `$dynamicAnchor` or `$recursiveAnchor`.

 @see "The IR", in the guide.
*/
struct anchor {
    /** The resource it is a name in. */
    resource_id resource{};

    /** The name. */
    string_id name{};

    /** The node it names, which belongs to `resource`. */
    node_id target{};

    /** Whether it is dynamic: `$dynamicAnchor`, also a plain anchor, or `$recursiveAnchor`. */
    bool dynamic = false;
};

}  // namespace webcpp::json_schema::ir

#endif  // WEBCPP_JSON_SCHEMA_IR_NODE_HPP
